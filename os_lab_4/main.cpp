#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <string>
#include <vector>
#include <list>
#include <memory>
#include <chrono>
#include <unordered_map>
using namespace std;

#define VAP (1ULL<<32)
#define PGSize 4096
#define PGShift 12
#define TLBSize 128
#define TLBCycle 1
#define MACycle 200
#define DISK_CYCLES 20000000ULL

const uint32_t INVALID_FN = 0xFFFFFFFFu;
const uint64_t HASH_SEED = 14695981039346656037ULL;
const uint64_t HASH_PRIME = 1099511628211ULL;

// Frame allocation (Parts I-III): first touch gets the next free frame (0, 1, 2, ...)
class frame_alloc {
    uint32_t next = 0;
public:
    uint32_t alloc() { return next++; }
    uint64_t used() const { return next; }
};

// Physical memory (Parts I-III): data is accessed by frame number + offset
class phys_mem {
public:
    void access(uint32_t fn, uint64_t offset, uint64_t& time_taken) {
        uint64_t paddr = ((uint64_t)fn << PGShift) | offset;
        (void)paddr;               // nothing to store in this simulation
        time_taken += MACycle;     // data access
    }
};

// Part IV: limited physical memory with LRU replacement.
// Doubly linked list over frame numbers (O(1) ops): head = MRU, tail = LRU.
class phys_mem_lru {
    uint64_t n_frames;
    uint64_t next_free = 0;             // frames handed out so far (cold start)
    vector<uint32_t> prv, nxt;          // linked list links, indexed by frame
    vector<uint64_t> owner;             // frame -> vpn currently stored in it
    uint32_t head = INVALID_FN, tail = INVALID_FN;

    void unlink(uint32_t fn) {
        uint32_t p = prv[fn], n = nxt[fn];
        if (p != INVALID_FN) nxt[p] = n; else head = n;
        if (n != INVALID_FN) prv[n] = p; else tail = p;
    }
    void push_front(uint32_t fn) {
        prv[fn] = INVALID_FN; nxt[fn] = head;
        if (head != INVALID_FN) prv[head] = fn;
        head = fn;
        if (tail == INVALID_FN) tail = fn;
    }
    void touch(uint32_t fn) {
        if (head == fn) return;
        unlink(fn);
        push_front(fn);
    }
public:
    phys_mem_lru(uint64_t frames)
        : n_frames(frames), prv(frames, INVALID_FN), nxt(frames, INVALID_FN), owner(frames, 0) {
    }

    // Page fault: give vpn a frame. Free frame -> no cost. Otherwise evict LRU page (+20M cycles).
    uint32_t alloc(uint64_t vpn, uint64_t& time_taken, uint64_t& victim, bool& evicted) {
        uint32_t fn;
        if (next_free < n_frames) {
            fn = (uint32_t)next_free++;
            evicted = false;
        }
        else {
            fn = tail;                          // LRU frame
            victim = owner[fn];
            evicted = true;
            time_taken += DISK_CYCLES;
            unlink(fn);
        }
        owner[fn] = vpn;
        push_front(fn);
        return fn;
    }

    // Called on EVERY access (TLB hit or miss): update LRU + charge data access
    void access(uint32_t fn, uint64_t offset, uint64_t& time_taken) {
        touch(fn);
        uint64_t paddr = ((uint64_t)fn << PGShift) | offset;
        (void)paddr;
        time_taken += MACycle;
    }
    uint64_t used() const { return next_free; }
    uint64_t size() const { return n_frames; }
};

// pgtb1: one flat array of entries (the building block)
class pgtb1 {
    vector<uint32_t> entry;        // index -> frame number
    vector<uint64_t> count;        // index -> number of accesses
    uint64_t n_accessed = 0;       // distinct entries touched
public:
    pgtb1(int bits) : entry(1u << bits, INVALID_FN), count(1u << bits, 0) {}

    // Parts I-III: one memory access to read the entry; allocates a frame on first touch
    uint32_t translate(uint64_t idx, frame_alloc& fa, uint64_t& time_taken) {
        time_taken += MACycle;
        if (count[idx]++ == 0) n_accessed++;
        if (entry[idx] == INVALID_FN) entry[idx] = fa.alloc();
        return entry[idx];
    }

    // Part IV: walk only (one memory access). Does NOT allocate a frame.
    // Returns INVALID_FN if the page is not present in memory.
    uint32_t walk(uint64_t idx, uint64_t& time_taken) {
        time_taken += MACycle;
        if (count[idx]++ == 0) n_accessed++;
        return entry[idx];
    }
    void install(uint64_t idx, uint32_t fn) { entry[idx] = fn; }
    void invalidate(uint64_t idx) { entry[idx] = INVALID_FN; }

    uint64_t allocated() const { return entry.size(); }
    uint64_t accessed()  const { return n_accessed; }
    const vector<uint64_t>& counts() const { return count; }
};

// Common interface so the three functions can share reporting code
class ptable {
public:
    virtual ~ptable() {}
    virtual uint32_t translate(uint64_t vpn, uint64_t& time_taken) = 0;
    virtual uint64_t frames_used() const = 0;
    virtual uint64_t allocated() const = 0;
    virtual uint64_t accessed() const = 0;
    // writes "vpn,count" rows (ascending vpn, only pages with count > 0); returns rows written
    virtual uint64_t dump_counts(FILE* out) const = 0;
};

// Single level: one pgtb1 with 2^20 entries, all allocated up front
class pgtb_single : public ptable {
    pgtb1 leaf;
    frame_alloc fa;
public:
    pgtb_single() : leaf(20) {}
    uint32_t translate(uint64_t vpn, uint64_t& t) override { return leaf.translate(vpn, fa, t); }
    uint64_t frames_used() const override { return fa.used(); }
    uint64_t allocated() const override { return leaf.allocated(); }
    uint64_t accessed() const override { return leaf.accessed(); }
    uint64_t dump_counts(FILE* out) const override {
        const vector<uint64_t>& c = leaf.counts();
        uint64_t rows = 0;
        for (size_t i = 0; i < c.size(); i++)
            if (c[i]) { fprintf(out, "%zu,%llu\n", i, (unsigned long long)c[i]); rows++; }
        return rows;
    }
};

// Two level: 10 bits outer, 10 bits inner. Inner tables are pgtb1(10), made on demand.
class pgtb2 : public ptable {
    vector<unique_ptr<pgtb1>> outer;
    frame_alloc fa;
    uint64_t n_inner = 0;
public:
    pgtb2() : outer(1u << 10) {}

    uint32_t translate(uint64_t vpn, uint64_t& t) override {
        t += MACycle;                                   // outer table access
        uint64_t hi = vpn >> 10, lo = vpn & 0x3FF;
        if (!outer[hi]) { outer[hi].reset(new pgtb1(10)); n_inner++; }
        return outer[hi]->translate(lo, fa, t);         // inner table access
    }

    // Part IV: two-level walk (outer access + inner access), no frame allocation
    uint32_t walk(uint64_t vpn, uint64_t& t) {
        t += MACycle;                                   // outer table access
        uint64_t hi = vpn >> 10, lo = vpn & 0x3FF;
        if (!outer[hi]) { outer[hi].reset(new pgtb1(10)); n_inner++; }
        return outer[hi]->walk(lo, t);                  // inner table access
    }
    void install(uint64_t vpn, uint32_t fn) { outer[vpn >> 10]->install(vpn & 0x3FF, fn); }
    void invalidate(uint64_t vpn) {                     // inner table stays allocated
        if (outer[vpn >> 10]) outer[vpn >> 10]->invalidate(vpn & 0x3FF);
    }

    uint64_t frames_used() const override { return fa.used(); }
    uint64_t allocated() const override { return 1024 + n_inner * 1024; }
    uint64_t accessed() const override {                // leaf entries touched + outer entries used
        uint64_t s = n_inner;
        for (size_t i = 0; i < outer.size(); i++) if (outer[i]) s += outer[i]->accessed();
        return s;
    }
    uint64_t dump_counts(FILE* out) const override {
        uint64_t rows = 0;
        for (size_t hi = 0; hi < outer.size(); hi++) {
            if (!outer[hi]) continue;
            const vector<uint64_t>& c = outer[hi]->counts();
            for (size_t lo = 0; lo < c.size(); lo++)
                if (c[lo]) {
                    fprintf(out, "%zu,%llu\n", (hi << 10) | lo, (unsigned long long)c[lo]);
                    rows++;
                }
        }
        return rows;
    }
};

// TLB: fully associative, LRU, 128 entries
class tlb {
    list<pair<uint64_t, uint32_t>> lru;   // front = most recently used
    unordered_map<uint64_t, list<pair<uint64_t, uint32_t>>::iterator> mp;
public:
    bool lookup(uint64_t vpn, uint32_t& fn) {
        auto it = mp.find(vpn);
        if (it == mp.end()) return false;
        lru.splice(lru.begin(), lru, it->second);   // move to front
        fn = it->second->second;
        return true;
    }
    void insert(uint64_t vpn, uint32_t fn) {        // call only after a miss
        if (mp.size() == TLBSize) { mp.erase(lru.back().first); lru.pop_back(); }
        lru.emplace_front(vpn, fn);
        mp[vpn] = lru.begin();
    }
    // Part IV: remove a stale entry when its page is evicted (no cycles, other LRU order untouched)
    void invalidate(uint64_t vpn) {
        auto it = mp.find(vpn);
        if (it == mp.end()) return;                 // not in TLB, nothing to do
        lru.erase(it->second);
        mp.erase(it);
    }
};

static void print_report(const char* name, const ptable& pt, uint64_t n,
    uint64_t time_taken, uint64_t hash, bool use_tlb, uint64_t hits) {
    if (use_tlb) {
        uint64_t misses = n - hits;
        cout << "== " << name << " ==\n";
        cout << "  TLB hits:       " << hits << "\n"
            << "  cycles:         " << time_taken << "\n"
            << "  TLB misses:     " << misses << "\n"
            << "  TLB hit rate:   " << (n ? (double)hits / n : 0.0) << "\n"
            << "  TLB miss rate:  " << (n ? (double)misses / n : 0.0) << "\n";
        cout << "  hash:           " << hash << endl;
        return;
    }
    cout << "== " << name << " ==\n"
        << "  accesses:       " << n << "\n"
        << "  frames used:    " << pt.frames_used() << "\n"
        << "  PT utilization: " << pt.accessed() << " / " << pt.allocated()
        << " = " << (double)pt.accessed() / pt.allocated() << "\n";
    cout << "  hash:           " << hash << endl;

}

// ---- CSV helpers (deliverable B): one file per trace, so different inputs never overwrite each other ----
// "/data/traces/gcc.trace" -> "gcc"  (handles both / and \ separators, strips the last extension)
static string trace_base(const char* path) {
    string s(path);
    size_t slash = s.find_last_of("/\\");
    if (slash != string::npos) s = s.substr(slash + 1);
    size_t dot = s.find_last_of('.');
    if (dot != string::npos && dot > 0) s = s.substr(0, dot);
    if (s.empty()) s = "trace";
    return s;
}

// Writes "<base>_<suffix>.csv" with header "vpn,count". Returns false on failure.
static bool write_counts_csv(const char* trace_path, const char* suffix, const ptable& pt) {
    string name = trace_base(trace_path) + "_" + suffix + ".csv";
    FILE* out = fopen(name.c_str(), "w");
    if (!out) { perror(("cannot write " + name).c_str()); return false; }
    fprintf(out, "vpn,count\n");
    uint64_t rows = pt.dump_counts(out);
    if (fclose(out) != 0) { perror("fclose"); return false; }
    cout << "  wrote " << name << " (" << rows << " pages touched)" << endl;
    return true;
}

// ---------------- Part I: single-level page table, no TLB ----------------
uint64_t single_p1(const char* path_to_file, uint64_t max_lines) {
    FILE* f = fopen(path_to_file, "r");
    if (!f) { perror("fopen"); return 0; }

    char line[256];
    unsigned long long a;
    char type, rw;
    uint64_t shown = 0, n = 0;
    bool verbose = max_lines && max_lines <= 20;

    pgtb_single pt;
    phys_mem mem;
    uint64_t time_taken = 0;
    uint64_t hash = HASH_SEED;

    while (fgets(line, sizeof line, f) && (max_lines == 0 || shown++ < max_lines)) {
        if (sscanf(line, "%llu,%c,%c", &a, &type, &rw) != 3) continue;
        uint64_t addr = a & 0xFFFFFFFFULL;
        uint64_t vpn = addr >> PGShift;
        uint64_t offset = addr & (PGSize - 1);
        n++;

        uint32_t fn = pt.translate(vpn, time_taken);   // step 1: vpn -> fn
        hash = (hash ^ fn) * HASH_PRIME;               // step 2: update hash
        mem.access(fn, offset, time_taken);            // step 3: data access

        if (verbose)
            cout << "vaddr=" << addr << " vpn=" << vpn << " fn=" << fn
            << " " << type << " " << rw << endl;
    }
    fclose(f);

    print_report("single-level, no TLB", pt, n, time_taken, hash, false, 0);
    write_counts_csv(path_to_file, "page_counts_1level", pt);   // hot/cold plot data
    return hash;
}

// ---------------- Part II: two-level page table, no TLB ----------------
uint64_t single_p2(const char* path_to_file, uint64_t max_lines) {
    FILE* f = fopen(path_to_file, "r");
    if (!f) { perror("fopen"); return 0; }

    char line[256];
    unsigned long long a;
    char type, rw;
    uint64_t shown = 0, n = 0;
    bool verbose = max_lines && max_lines <= 20;

    pgtb2 pt;
    phys_mem mem;
    uint64_t time_taken = 0;
    uint64_t hash = HASH_SEED;

    while (fgets(line, sizeof line, f) && (max_lines == 0 || shown++ < max_lines)) {
        if (sscanf(line, "%llu,%c,%c", &a, &type, &rw) != 3) continue;
        uint64_t addr = a & 0xFFFFFFFFULL;
        uint64_t vpn = addr >> PGShift;
        uint64_t offset = addr & (PGSize - 1);
        n++;

        uint32_t fn = pt.translate(vpn, time_taken);   // step 1: vpn -> fn (2 memory accesses)
        hash = (hash ^ fn) * HASH_PRIME;               // step 2: update hash
        mem.access(fn, offset, time_taken);            // step 3: data access

        if (verbose)
            cout << "vaddr=" << addr << " vpn=" << vpn << " fn=" << fn
            << " " << type << " " << rw << endl;
    }
    fclose(f);

    print_report("two-level, no TLB", pt, n, time_taken, hash, false, 0);
    write_counts_csv(path_to_file, "page_counts_2level", pt);   // should equal the 1-level CSV
    return hash;
}

// ---------------- Part III: TLB + page table (single-level or two-level) ----------------
uint64_t tlb_p3(const char* path_to_file, bool two_level, uint64_t max_lines) {
    FILE* f = fopen(path_to_file, "r");
    if (!f) { perror("fopen"); return 0; }

    char line[256];
    unsigned long long a;
    char type, rw;
    uint64_t shown = 0, n = 0, hits = 0;
    bool verbose = max_lines && max_lines <= 20;

    unique_ptr<ptable> pt;
    if (two_level) pt.reset(new pgtb2());
    else           pt.reset(new pgtb_single());
    tlb t;
    phys_mem mem;
    uint64_t time_taken = 0;
    uint64_t hash = HASH_SEED;

    while (fgets(line, sizeof line, f) && (max_lines == 0 || shown++ < max_lines)) {
        if (sscanf(line, "%llu,%c,%c", &a, &type, &rw) != 3) continue;
        uint64_t addr = a & 0xFFFFFFFFULL;
        uint64_t vpn = addr >> PGShift;
        uint64_t offset = addr & (PGSize - 1);
        n++;

        uint32_t fn;
        time_taken += TLBCycle;                        // TLB lookup (hit or miss)
        if (t.lookup(vpn, fn)) {
            hits++;                                    // hit: no page table walk
        }
        else {
            fn = pt->translate(vpn, time_taken);       // miss: walk the page table
            t.insert(vpn, fn);
        }
        hash = (hash ^ fn) * HASH_PRIME;               // update hash on hits AND misses
        mem.access(fn, offset, time_taken);            // data access

        if (verbose)
            cout << "vaddr=" << addr << " vpn=" << vpn << " fn=" << fn
            << " " << type << " " << rw << endl;
    }
    fclose(f);

    print_report(two_level ? "two-level + TLB" : "single-level + TLB",
        *pt, n, time_taken, hash, true, hits);

    return hash;
}

// ---------------- Part IV: two-level + TLB + limited physical memory (LRU) ----------------
uint64_t part4(const char* path_to_file, double fraction, uint64_t max_lines) {
    uint64_t frames = (uint64_t)(VAP * fraction) / PGSize;
    if (frames == 0) { cerr << "frames must be >= 1" << endl; return 0; }
    FILE* f = fopen(path_to_file, "r");
    if (!f) { perror("fopen"); return 0; }

    char line[256];
    unsigned long long a;
    char type, rw;
    uint64_t shown = 0, n = 0, hits = 0, faults = 0, replacements = 0;
    bool verbose = max_lines && max_lines <= 20;

    pgtb2 pt;
    tlb t;
    phys_mem_lru mem(frames);
    uint64_t time_taken = 0;
    uint64_t hash = HASH_SEED;

    while (fgets(line, sizeof line, f) && (max_lines == 0 || shown++ < max_lines)) {
        if (sscanf(line, "%llu,%c,%c", &a, &type, &rw) != 3) continue;
        uint64_t addr = a & 0xFFFFFFFFULL;
        uint64_t vpn = addr >> PGShift;
        uint64_t offset = addr & (PGSize - 1);
        n++;

        uint32_t fn;
        bool fault = false, evicted = false;
        uint64_t victim = 0;

        time_taken += TLBCycle;                        // TLB lookup (hit or miss)
        if (t.lookup(vpn, fn)) {
            hits++;                                    // hit: page is in memory for sure
        }
        else {
            fn = pt.walk(vpn, time_taken);             // miss: walk (charged even if invalid)
            if (fn == INVALID_FN) {                    // page fault
                fault = true;
                faults++;
                fn = mem.alloc(vpn, time_taken, victim, evicted);   // +20M only if it evicts
                if (evicted) {
                    replacements++;
                    pt.invalidate(victim);             // victim no longer in memory
                    t.invalidate(victim);              // drop stale TLB entry
                }
                pt.install(vpn, fn);
            }
            t.insert(vpn, fn);
        }
        hash = (hash ^ fn) * HASH_PRIME;               // update hash on every access
        mem.access(fn, offset, time_taken);            // LRU update + data access

        if (verbose) {
            cout << "vaddr=" << addr << " vpn=" << vpn << " fn=" << fn;
            if (fault) cout << " FAULT";
            if (evicted) cout << " EVICT vpn=" << victim;
            cout << endl;
        }
    }
    fclose(f);
    cout << "== Part IV: two-level + TLB, memory = " << fraction << " x VAP, frames = " << frames << " ==\n"
        << "  accesses:            " << n << "\n"
        << "  frames used:         " << mem.used() << "\n"
        << "  page faults (all):   " << faults << "  rate = " << (n ? (double)faults / n : 0.0) << "\n"
        << "  replacements:        " << replacements << "  rate = " << (n ? (double)replacements / n : 0.0) << "\n"
        << "  TLB hits:            " << hits << "\n"
        << "  TLB misses:          " << (n - hits) << "\n"
        << "  TLB hit rate:        " << (n ? (double)hits / n : 0.0) << "\n"
        << "  TLB miss rate:       " << (n ? (double)(n - hits) / n : 0.0) << "\n"
        << "  cycles:              " << time_taken << "\n"
        << "  hash:                " << hash << endl;

    return hash;
}

// Runs fn, prints elapsed wall-clock seconds, returns fn's result (the hash)
template <class F>
uint64_t timed(const char* label, F fn) {
    auto t0 = chrono::steady_clock::now();
    uint64_t r = fn();
    double s = chrono::duration<double>(chrono::steady_clock::now() - t0).count();
    cout << "  [time] " << label << ": " << s << " s\n" << endl;
    return r;
}

static void usage(const char* prog) {
    cerr << "usage: " << prog << " <part1|part2|part3|part4> <trace file> [max_lines]\n"
        << "  part1 : single-level page table\n"
        << "  part2 : two-level page table\n"
        << "  part3 : TLB + single-level and TLB + two-level\n"
        << "  part4 : two-level + TLB + LRU, memory = 1/2, 1/4, 1/8, 1/512, 1/1024, 1/16384 x VAP\n"
        << "  max_lines (optional): only process the first N lines (0 = whole file)\n";
}

int main(int argc, char** argv) {
    if (argc < 3) { usage(argv[0]); return 1; }

    string part = argv[1];
    const char* trace = argv[2];
    uint64_t max_lines = (argc >= 4) ? strtoull(argv[3], nullptr, 10) : 0;

    if (part == "part1" || part == "1") {
        timed("part1", [&] { return single_p1(trace, max_lines); });
    }
    else if (part == "part2" || part == "2") {
        timed("part2", [&] { return single_p2(trace, max_lines); });
    }
    else if (part == "part3" || part == "3") {
        timed("part3 single-level", [&] { return tlb_p3(trace, false, max_lines); });
        timed("part3 two-level", [&] { return tlb_p3(trace, true, max_lines); });
    }
    else if (part == "part4" || part == "4") {
        const double fractions[] = { 1.0 / 2, 1.0 / 4, 1.0 / 8, 1.0 / 512, 1.0 / 1024, 1.0 / 16384 };
        for (double fr : fractions)
            timed("part4", [&] { return part4(trace, fr, max_lines); });
    }
    else {
        cerr << "unknown part: " << part << "\n";
        usage(argv[0]);
        return 1;
    }
    return 0;
}

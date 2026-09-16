#include "cpu.hpp"

cpu::cpu(int cpu_id)
{
	this->cpu_id = cpu_id;
	isUsed = false;
	time_stamp = 0;
}

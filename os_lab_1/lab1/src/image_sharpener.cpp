#include <iostream>
#include "libppm.h"
#include <algorithm>
#include <string>
#include <cstdint>
#include <chrono>
#include <iomanip>

using namespace std;
using namespace chrono;

void take_av(struct image_t *input_image,struct image_t *smoothen_img, int i , int j){
	
	for(int a = -1 ; a <= 1 ; a++){
		for( int b = -1; b <= 1 ; b++){
			smoothen_img->image_pixels[i][j][0] += input_image->image_pixels[i+a][j+b][0]/9;
			smoothen_img->image_pixels[i][j][1] += input_image->image_pixels[i+a][j+b][1]/9;
			smoothen_img->image_pixels[i][j][2] += input_image->image_pixels[i+a][j+b][2]/9;
		}
	}
} 

struct image_t* S1_smoothen(struct image_t *input_image)
{

	// tasks :
	
	struct image_t* smoothen_img = new struct image_t;
	
	smoothen_img->height = input_image->height;
	smoothen_img->width = input_image->width;

	smoothen_img->image_pixels = new uint8_t**[smoothen_img->height];
	
	for(int i = 0; i < smoothen_img->height; i++)
	{
		smoothen_img->image_pixels[i] = new uint8_t*[smoothen_img->width];
		for(int j = 0; j < smoothen_img->width; j++){
			smoothen_img->image_pixels[i][j] = new uint8_t[3];
			if( (i > 0) && (i < smoothen_img->height-1) && (j > 0) && (j < smoothen_img->width-1)){
				take_av(input_image, smoothen_img,i,j);
			}
			else {
				smoothen_img->image_pixels[i][j][0] = smoothen_img->image_pixels[i][j][1] = smoothen_img->image_pixels[i][j][2] = 0;
			}
		}
	}

	return smoothen_img;
}

struct image_t* S2_find_details(struct image_t *input_image, struct image_t *smoothened_image)
{
	struct image_t* detail_img = new struct image_t;
	
	detail_img->height = input_image->height;
	detail_img->width = input_image->width;

	detail_img->image_pixels = new uint8_t**[detail_img->height];
	
	for(int i = 0; i < detail_img->height; i++)
	{
		detail_img->image_pixels[i] = new uint8_t*[detail_img->width];
		for(int j = 0; j < detail_img->width; j++){
			detail_img->image_pixels[i][j] = new uint8_t[3];
			
			detail_img->image_pixels[i][j][0] = std::clamp(input_image->image_pixels[i][j][0] - smoothened_image->image_pixels[i][j][0],0,255);
			detail_img->image_pixels[i][j][1] = std::clamp(input_image->image_pixels[i][j][1] - smoothened_image->image_pixels[i][j][1],0,255);
			detail_img->image_pixels[i][j][2] = std::clamp(input_image->image_pixels[i][j][2] - smoothened_image->image_pixels[i][j][2],0,255);
		}
	}
	
	return detail_img;
}

struct image_t* S3_sharpen(struct image_t *input_image, struct image_t *details_image)
{
	struct image_t* sharpen_img = new struct image_t;
	
	sharpen_img->height = input_image->height;
	sharpen_img->width = input_image->width;

	sharpen_img->image_pixels = new uint8_t**[sharpen_img->height];
	
	for(int i = 0; i < sharpen_img->height; i++)
	{
		sharpen_img->image_pixels[i] = new uint8_t*[sharpen_img->width];
		for(int j = 0; j < sharpen_img->width; j++){
			sharpen_img->image_pixels[i][j] = new uint8_t[3];
			float a = 1;
			sharpen_img->image_pixels[i][j][0] = std::clamp(input_image->image_pixels[i][j][0] + static_cast<int>(a*details_image->image_pixels[i][j][0]),0,255);	
			sharpen_img->image_pixels[i][j][1] = std::clamp(input_image->image_pixels[i][j][1] + static_cast<int>(a*details_image->image_pixels[i][j][1]),0,255);
			sharpen_img->image_pixels[i][j][2] = std::clamp(input_image->image_pixels[i][j][2] + static_cast<int>(a*details_image->image_pixels[i][j][2]),0,255);		sharpen_img->image_pixels[i][j][0] = std::clamp(input_image->image_pixels[i][j][0] + static_cast<int>(a*details_image->image_pixels[i][j][0]),0,255);

		}
	}

	return sharpen_img;
}

int main(int argc, char **argv)
{
	/*if(argc != 3)
	{
		cout << "usage: ./a.out <path-to-original-image> <path-to-transformed-image>\n\n";
		exit(0);
	}*/
	
	vector<vector<double>> av(7, vector<double>(5,0.0f));

	for (int j = 0; j < 5; j++) {
		cout << "RUN : " << j + 1 << endl;
		for (int i = 0; i < 7; i++) {


			string input = "../images/" + to_string(i + 1) + ".ppm";
			string output = "../images/" + to_string(i + 1) + "_output.ppm";

			const auto processStart{ steady_clock::now() };

			struct image_t* input_image = read_ppm_file(const_cast<char*>(input.c_str()));
			const auto inputFileRead{ steady_clock::now() };
			const duration<double> t1{ inputFileRead - processStart };

			struct image_t* smoothened_image = S1_smoothen(input_image);
			const auto S1{ steady_clock::now() };
			const duration<double> t2{ S1 - processStart };

			struct image_t* details_image = S2_find_details(input_image, smoothened_image);
			const auto S2{ steady_clock::now() };
			const duration<double> t3{ S2 - processStart };

			struct image_t* sharpened_image = S3_sharpen(input_image, details_image);
			const auto S3{ steady_clock::now() };
			const duration<double> t4{ S3 - processStart };

			write_ppm_file(const_cast<char*>(output.c_str()), sharpened_image);
			const auto outputFileWrite{ steady_clock::now() };
			const duration<double> t5{ outputFileWrite - processStart };

			cout << t1 << " " << t2 << " " << t3 << " " << t4 << " " << t5 << endl;
			av[i][0] += static_cast<double>(t1.count() / 5);
			av[i][1] += static_cast<double>(t2.count() / 5);
			av[i][2] += static_cast<double>(t3.count() / 5);
			av[i][3] += static_cast<double>(t4.count() / 5);
			av[i][4] += static_cast<double>(t5.count() / 5);
		}
	}

	cout << "AVERAGES : " << '\n';

	for (int i = 0; i < 7; i++) {
		for (int j = 0; j < 5; j++) {
			cout << fixed << setprecision(3) << av[i][j] << " ";
		}
		cout << '\n';
	}

	return 0;
}

#include "HDR\HDRImage.h"

float sinc(float x) {               /* Supporting sinc function */
  if (fabs(x) < 1.0e-4) return 1.0 ;
  else return(sin(x)/x) ;
}

HDRImage::HDRImage(int width, int height) {

	this->width = width;
	this->height = height;
	this->cartesianCoord = (float*)malloc(width * height * 3 * sizeof(float));
	this->sphericalCoord = (float*)malloc(width * height * 2 * sizeof(float));
	this->domegaProduct = (float*)malloc(width * height * 9 * sizeof(float));
	this->image = (float*)malloc(width * height * 3 * sizeof(float));

	for(int sh = 0; sh < 9; sh++)
		for(int ch = 0; ch < 3; ch++)
			SHCoeffs[sh * 3 + ch] = 0;

}

HDRImage::~HDRImage() {
	
	delete [] cartesianCoord;
	delete [] sphericalCoord;
	delete [] domegaProduct;
	delete [] image;

}

void HDRImage::computeCoordinates() {
	
	int pixel;
	float u, v, r;
	float midWidth = width/2;
	float midHeight = height/2;

	for(int y = 0; y < height; y++) {
		for(int x = 0; x < width; x++) {
			
			pixel = y * width + x;
			u = (x - midWidth)/midWidth;
			v = (y - midHeight)/midHeight;
			r = sqrtf(u * u + v * v);

			if(r > 1.0) {
			
				cartesianCoord[pixel * 3 + 0] = 0;
				cartesianCoord[pixel * 3 + 1] = 0;
				cartesianCoord[pixel * 3 + 2] = 0;
				sphericalCoord[pixel * 2 + 0] = 0;
				sphericalCoord[pixel * 2 + 1] = 0;
			
			} else {
			
				float phi = atan2(v, u);
				float theta = PI * r;

				if(theta != theta) theta = 0;
				if(phi != phi) phi = 0;

				sphericalCoord[pixel * 2 + 0] = theta;
				sphericalCoord[pixel * 2 + 1] = phi;
				cartesianCoord[pixel * 3 + 0] = sin(theta) * cos(phi);
				cartesianCoord[pixel * 3 + 1] = sin(theta) * sin(phi);
				cartesianCoord[pixel * 3 + 2] = cos(theta);
				
			}

		}
	}

}

void HDRImage::computeDomegaProduct() {

	float dx, dy, dz, theta, domega, c;

	for(int pixel = 0; pixel < width * height; pixel++) {
		
		theta = sphericalCoord[pixel * 2 + 0]; 
		dx = cartesianCoord[pixel * 3 + 0];
		dy = cartesianCoord[pixel * 3 + 1];
		dz = cartesianCoord[pixel * 3 + 2];
		domega = (2*PI/(float)width)*(2*PI/(float)width)*sinc(theta);
		c = 0.282095;
		domegaProduct[pixel * 9 + 0] = c * domega;
		c = 0.488603;
		domegaProduct[pixel * 9 + 1] = c * dy * domega;
		domegaProduct[pixel * 9 + 2] = c * dz * domega;
		domegaProduct[pixel * 9 + 3] = c * dx * domega;
		c = 1.092548;
		domegaProduct[pixel * 9 + 4] = c * dx * dy * domega;
		domegaProduct[pixel * 9 + 5] = c * dy * dz * domega;
		domegaProduct[pixel * 9 + 7] = c * dx * dz * domega;
		c = 0.315392;
		domegaProduct[pixel * 9 + 6] = c * (3 * dz * dz - 1) * domega;
		c = 0.546274;
		domegaProduct[pixel * 9 + 8] = c * (dx * dx - dy * dy) * domega;
	
	}

}

void HDRImage::computeSHCoeffs() {
	
	float c;
	int pixel;
	float domega, theta, imageValue;
	float dx, dy, dz, u, v, r;
	float midWidth = width/2;
	float midHeight = height/2;

	for(int sh = 0; sh < 9; sh++)
		for(int ch = 0; ch < 3; ch++)
			SHCoeffs[sh * 3 + ch] = 0;


	for(int y = 0; y < height; y++) {
		for(int x = 0; x < width; x++) {
			
			pixel = y * width + x;
			u = (x - midWidth)/midWidth;
			v = (y - midHeight)/midHeight;
			r = sqrtf(u * u + v * v);
			if(r > 1.0) continue;

			for(int ch = 0; ch < 3; ch++) {
				imageValue = image[pixel * 3 + ch];
				c = 0.282095;
				SHCoeffs[0 * 3 + ch] += imageValue * domegaProduct[pixel * 9 + 0];
				c = 0.488603;
				SHCoeffs[1 * 3 + ch] += imageValue * domegaProduct[pixel * 9 + 1];
				SHCoeffs[2 * 3 + ch] += imageValue * domegaProduct[pixel * 9 + 2];
				SHCoeffs[3 * 3 + ch] += imageValue * domegaProduct[pixel * 9 + 3];
				c = 1.092548;
				SHCoeffs[4 * 3 + ch] += imageValue * domegaProduct[pixel * 9 + 4];
				SHCoeffs[5 * 3 + ch] += imageValue * domegaProduct[pixel * 9 + 5];
				SHCoeffs[7 * 3 + ch] += imageValue * domegaProduct[pixel * 9 + 7];
				c = 0.315392;
				SHCoeffs[6 * 3 + ch] += imageValue * domegaProduct[pixel * 9 + 6];
				c = 0.546274;
				SHCoeffs[8 * 3 + ch] += imageValue * domegaProduct[pixel * 9 + 8];
			}

		}
	}

	for(int sh = 0; sh < 9; sh++)
		for(int ch = 0; ch < 3; ch++)
			SHCoeffs[sh * 3 + ch] *= scale;

}

void HDRImage::computeSphericalMap() {
	
	float c[5];
	c[0] = 0.429043;
	c[1] = 0.511664;
	c[2] = 0.743125;
	c[3] = 0.886227;
	c[4] = 0.247708;

	float dx, dy, dz;
	for(int pixel = 0; pixel < width * height; pixel++) {
		
		int x = pixel % width;
		int y = pixel / width;

		float nx = x/(float)width;
		float ny = y/(float)height;

		dx = cartesianCoord[pixel * 3 + 0];
		dy = cartesianCoord[pixel * 3 + 1];
		dz = cartesianCoord[pixel * 3 + 2];
	
		if(dx == 0 && dy == 0 && dz == 0) {

			for(int ch = 0; ch < 3; ch++)
				image[pixel * 3 + ch] = 0;
		
		} else {

			for(int ch = 0; ch < 3; ch++) {
				image[pixel * 3 + ch] = c[0] * SHCoeffs[8 * 3 + ch] * (dx * dx - dy * dy) + 
					c[2] * SHCoeffs[6 * 3 + ch] * dz * dz +
					c[3] * SHCoeffs[0 * 3 + ch] -
					c[4] * SHCoeffs[6 * 3 + ch] +
					2 * c[0] * (SHCoeffs[4 * 3 + ch] * dx * dy + SHCoeffs[7 * 3 + ch] * dx * dz + SHCoeffs[5 * 3 + ch] * dy * dz) +
					2 * c[1] * (SHCoeffs[3 * 3 + ch] * dx + SHCoeffs[1 * 3 + ch] * dy + SHCoeffs[2 * 3 + ch] * dz);
				if(image[pixel * 3 + ch] < 0) image[pixel * 3 + ch] = 0;
			}

		}

	}
	
}

void HDRImage::load(float *image) {
	
	memcpy(this->image, image, width * height * 3 * sizeof(float));

}

void HDRImage::load(unsigned char *image) {
	
	for(int pixel = 0; pixel < width * height; pixel++)
		for(int ch = 0; ch < 3; ch++)
			this->image[pixel * 3 + ch] = image[pixel * 3 + ch]/255.f;

}
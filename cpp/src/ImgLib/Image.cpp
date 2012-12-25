#include "Image.hpp"
#include "../LodePNG/lodepng.h"
#include "../NanoJPEG/jpeg_decoder.h"
#include <cassert>



namespace ImgLib {

	Image :: Image() :
		hasAlpha(false),
		hasAlphaDefault(false),
		width(0),
		height(0),
		pixels()
	{
	}
	Image :: Image(const Image& other) :
		hasAlpha(other.hasAlpha),
		hasAlphaDefault(other.hasAlphaDefault),
		width(other.width),
		height(other.height),
		pixels(other.pixels)
	{
	}
	Image :: ~Image() {
	}
	bool Image :: loadFromSource(const std::vector<unsigned char>* source, bool isPng, int colorDepthOverride, std::ostream* errorStream) {
		assert(source != NULL);

		// Load
		this->pixels.clear();
		this->width = 0;
		this->height = 0;


		if (isPng) {
			// Load the PNG
			lodepng::State state;
			state.info_raw.colortype = LCT_RGBA;

			unsigned int error = lodepng::decode(this->pixels, this->width, this->height, state, *source);

			// Error?
			if (error != 0) {
				this->width = 0;
				this->height = 0;
				this->pixels.clear();

				if (errorStream != NULL) {
					*errorStream << lodepng_error_text(error);
				}

				return false;
			}

			// Settings
			// TODO : make this work properly (sometimes detects 3 channels as 4; see LCT_PALETTE)
			this->hasAlphaDefault = (state.info_png.color.colortype == LCT_RGBA || state.info_png.color.colortype == LCT_GREY_ALPHA || state.info_png.color.colortype == LCT_PALETTE);
		}
		else {
			Jpeg::Decoder d(&((*source)[0]), source->size(), malloc, free);

			// Error
			if (d.GetResult() != Jpeg::Decoder::OK) {
				if (errorStream != NULL) {
					switch (d.GetResult()) {
						case Jpeg::Decoder::NotAJpeg:
						{
							*errorStream << "Jpeg load error: not a jpeg";
						}
						break;
						case Jpeg::Decoder::Unsupported:
						{
							*errorStream << "Jpeg load error: unsupported";
						}
						break;
						case Jpeg::Decoder::OutOfMemory:
						{
							*errorStream << "Jpeg load error: out of memory";
						}
						break;
						case Jpeg::Decoder::InternalError:
						{
							*errorStream << "Jpeg load error: internal error";
						}
						break;
						case Jpeg::Decoder::SyntaxError:
						{
							*errorStream << "Jpeg load error: syntax error";
						}
						break;
						default:
						{
							*errorStream << "Jpeg load error " << d.GetResult();
						}
						break;
					}
				}

				return false;
			}

			// Transfer
			this->width = d.GetWidth();
			this->height = d.GetHeight();

			this->pixels.resize(this->width * this->height * 4);
			if (d.IsColor()) {
				for (unsigned int y = 0; y < this->height; ++y) {
					for (unsigned int x = 0; x < this->width; ++x) {
						this->pixels[(y * this->width + x) * 4 + 0] = d.GetImage()[(y * this->width + x) * 3 + 0];
						this->pixels[(y * this->width + x) * 4 + 1] = d.GetImage()[(y * this->width + x) * 3 + 1];
						this->pixels[(y * this->width + x) * 4 + 2] = d.GetImage()[(y * this->width + x) * 3 + 2];
						this->pixels[(y * this->width + x) * 4 + 3] = 255;
					}
				}
			}
			else {
				unsigned char c;
				for (unsigned int y = 0; y < this->height; ++y) {
					for (unsigned int x = 0; x < this->width; ++x) {
						c = d.GetImage()[(y * this->width + x) * 3 + 0];
						this->pixels[(y * this->width + x) * 4 + 0] = c;
						this->pixels[(y * this->width + x) * 4 + 1] = c;
						this->pixels[(y * this->width + x) * 4 + 2] = c;
						this->pixels[(y * this->width + x) * 4 + 3] = 255;
					}
				}
			}

			// Jpeg, so no alpha
			this->hasAlphaDefault = false;
		}

		// Override
		if (colorDepthOverride == 3 || colorDepthOverride == 4) {
			this->hasAlpha = (colorDepthOverride == 4);
		}
		else {
			this->hasAlpha = this->hasAlphaDefault;
		}

		// Okay
		return true;
	}
	bool Image :: saveToVector(std::vector<unsigned char>* source, lodepng::State* state, std::ostream* errorStream) const {
		assert(state != NULL);
		assert(source != NULL);

		// Setup
		source->clear();

		unsigned int error = lodepng::encode(*source, this->pixels, this->width, this->height, *state);
		// Error?
		if (error != 0) {
			if (errorStream != NULL) {
				*errorStream << lodepng_error_text(error);
			}

			return false;
		}

		// Okay
		return true;
	}

	void Image :: copy(const Image* other) {
		assert(other != NULL);

		this->hasAlpha = other->hasAlpha;
		this->hasAlphaDefault = other->hasAlphaDefault;
		this->width = other->width;
		this->height = other->height;
		this->pixels = other->pixels;
	}

	unsigned char Image :: getPixel(unsigned int x, unsigned int y, unsigned int component) const {
		return this->pixels[(x + y * this->width) * 4 + component];
	}
	void Image :: setPixel(unsigned int x, unsigned int y, unsigned int component, unsigned char value) {
		this->pixels[(x + y * this->width) * 4 + component] = value;
	}

	unsigned int Image :: getChannelCount() const {
		return 3 + this->hasAlpha;
	}
	unsigned int Image :: getDefaultChannelCount() const {
		return 3 + this->hasAlphaDefault;
	}
	unsigned int Image :: getWidth() const {
		return this->width;
	}
	unsigned int Image :: getHeight() const {
		return this->height;
	}

	const std::vector<unsigned char>* Image :: getPixels() const {
		return &this->pixels;
	}

	void Image :: downscale(unsigned int width, unsigned int height) {
		// Create with space
		std::vector<unsigned char> newImage(width * height * 4, 0);

		// Downscale
		bool fx, fy;
		double xscale = static_cast<double>(this->width) / width;
		double yscale = static_cast<double>(this->height) / height;
		double value, vcol, v;
		double dxs, dys, s;
		double xrange[2];
		double yrange[2];
		for (unsigned int y = 0; y < height; ++y) {
			for (unsigned int x = 0; x < width; ++x) {
				for (unsigned int c = 0; c < 4; ++c) {
					value = 0.0;
					xrange[0] = x * xscale;
					xrange[1] = (x + 1) * xscale;
					yrange[0] = y * yscale;
					yrange[1] = (y + 1) * yscale;

					dxs = 0.0;
					dys = 0.0;
					fx = true;
					for (double dx = xrange[0], dxn; ; dx = dxn) {
						dxn = dx + 1.0;

						vcol = 0.0;
						fy = true;
						for (double dy = yrange[0], dyn; ; dy = dyn) {
							dyn = dy + 1.0;

							v = this->pixels[
								(static_cast<int>(dx) + static_cast<int>(dy) * this->width) * 4 + c
							];
							
							if (dyn >= yrange[1]) {
								// end pixel
								s = (1.0 - (static_cast<int>(dyn) - dy));
								if (fx) dys += s;
								vcol += v * s;
								break;
							}
							else if (fy) {
								// start pixel
								s = (static_cast<int>(dyn) - dy);
								if (fx) dys += s;
								vcol += v * s;
								fy = false;
							}
							else {
								if (fx) dys += 1.0;
								vcol += v;
							}
						}

						if (dxn >= xrange[1]) {
							// end col
							s = (1.0 - (static_cast<int>(dxn) - dx));
							dxs += s;
							value += vcol * s;
							break;
						}
						else if (fx) {
							// start col
							s = (static_cast<int>(dxn) - dx);
							dxs += s;
							value += vcol * s;
							fx = false;
						}
						else {
							dxs += 1.0;
							value += vcol;
						}
					}
					value /= dxs * dys;
					newImage[(x + y * width) * 4 + c] = static_cast<int>(value + 0.5);
				}
			}
		}

		// Replace
		this->width = width;
		this->height = height;
		newImage.swap(this->pixels);
	}

};


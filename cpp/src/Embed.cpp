#include "Include.hpp"
#include <iostream>
#include <iomanip>
#include <string>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <sstream>
#include <cassert>
#include "ImgLib/Image.hpp"
#include "ImgLib/ImageWriter.hpp"
#include "LodePNG/lodepng.h"
using namespace std;
using namespace ImgLib;



bool arg2bool(cstring arg);
bool fullOptimizeEncode(Image* image, lodepng::State* state, std::vector<unsigned char>* pngOutput, ostream* immediateStream, ostream* statusStream, ostream* errorStream);



int main(int argc, char** argv) {
	// Settings
	int filesizeLimit = 0;
	int bitmask = 0;
	std::string outputFile = "";
	int channelCount = 0;
	bool randomizeAll = false;
	bool scatter = false;
	bool fullOptimize = false;
	bool downscale = false;
	std::string imageFile = "";
	std::vector<std::string> sources;

	// Flags
	bool canFlag = true;
	for (int i = 1; i < argc; ++i) {
		if (canFlag && argv[i][0] == '-') {
			if (strcmp(argv[i], "--") == 0) {
				canFlag = false;
			}
			else if (strcmp(argv[i], "-b") == 0 || strcmp(argv[i], "-bitmask") == 0) {
				if (++i >= argc) break;
				if (stricmp(argv[i], "auto") == 0) {
					bitmask = 0;
				}
				else {
					bitmask = atoi(argv[i]);
					if (bitmask <= 0 || bitmask > 8) {
						cout << "Warning: invalid bitmask value \"" << argv[i] << "\"" << endl;
						bitmask = 0;
					}
				}
			}
			else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "-size") == 0 || strcmp(argv[i], "-size_limit") == 0) {
				if (++i >= argc) break;
				int pos = 0;
				while (argv[i][pos] != '\0') ++pos;

				int scale = 1;
				if ((argv[i][pos] & 0xDF) == 'K') scale = 1024;
				else if ((argv[i][pos] & 0xDF) == 'M') scale = 1024 * 1024;

				int value;
				if (scale == 1) {
					value = atoi(argv[i]);
				}
				else {
					char c = argv[i][pos];
					argv[i][pos] = '\0';
					value = atoi(argv[i]) * scale;
					argv[i][pos] = c;
				}

				if (value < 0) {
					cout << "Warning: file size limit cannot be negative: \"" << argv[i] << "\"" << endl;
				}
				else {
					filesizeLimit = value;
				}
			}
			else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "-out") == 0 || strcmp(argv[i], "-output") == 0) {
				if (++i >= argc) break;
				outputFile = argv[i];
				// .png extension
				int pos = outputFile.length();
				if (pos < 4 || (outputFile[pos - 4] != '.' || (outputFile[pos - 3] & 0xDF) != 'P' || (outputFile[pos - 2] & 0xDF) != 'N' || (outputFile[pos - 1] & 0xDF) != 'G')) {
					outputFile += ".png";
				}
			}
			else if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "-rand") == 0 || strcmp(argv[i], "-random") == 0 || strcmp(argv[i], "-randomize") == 0) {
				if (++i >= argc) break;
				randomizeAll = arg2bool(argv[i]);
			}
			else if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "-alpha") == 0) {
				if (++i >= argc) break;
				if (stricmp(argv[i], "auto") == 0) {
					channelCount = 0;
				}
				channelCount = (arg2bool(argv[i]) ? 4 : 3);
			}
			else if (strcmp(argv[i], "-O") == 0 || strcmp(argv[i], "-opt") == 0 || strcmp(argv[i], "-optimize") == 0) {
				if (++i >= argc) break;
				fullOptimize = arg2bool(argv[i]);
			}
			else if (strcmp(argv[i], "-S") == 0 || strcmp(argv[i], "-sc") == 0 || strcmp(argv[i], "-scatter") == 0) {
				if (++i >= argc) break;
				scatter = arg2bool(argv[i]);
			}
			else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "-down") == 0 || strcmp(argv[i], "-downscale") == 0 || strcmp(argv[i], "-scale") == 0) {
				if (++i >= argc) break;
				downscale = arg2bool(argv[i]);
			}
			else {
				cout << "Warning: unknown flag \"" << argv[i] << "\"" << endl;
			}
		}
		else {
			if (imageFile.length() == 0) {
				imageFile = argv[i];
			}
			else {
				sources.push_back(argv[i]);
			}
		}
	}

	// Usage
	if (sources.size() == 0 && imageFile.length() == 0) {
		cout << "Usage:" << endl;
		cout << "    " << argv[0] << " [-b ...] [-s ...] [-o ...] [-r ...] [-a ...] [-O ...] [-S ...] [-d ...] image.png file1.txt file2.txt ..." << endl;
		cout << "" << endl;
		cout << "    -b bitmask : set the amount of bits per color component to be used to store data" << endl;
		cout << "               : valid values are 1, 2, 3, 4, 5, 6, 7, 8, and auto" << endl;
		cout << "               : smaller values cause a smaller impact on the image, but need more space" << endl;
		cout << "" << endl;
		cout << "    -s size : set the amount of bits per color component to be used to store data" << endl;
		cout << "            : valid values are positive numbers, with a possible suffix of \"k\" or \"m\"" << endl;
		cout << "            : value of 0 indicates no size limit" << endl;
		cout << "            : example:  -s 3m" << endl;
		cout << "" << endl;
		cout << "    -o output : set the filename to output to" << endl;
		cout << "              : if the extension is not \".png\", a \".png\" will be appended" << endl;
		cout << "" << endl;
		cout << "    -r randomize : randomize pixels after data is stored" << endl;
		cout << "                 : when enabled, it maintains the \"fuzzy\" look of the image" << endl;
		cout << "                 : (i.e. no band of clean pixels as the end)" << endl;
		cout << "                 : values are \"1\", \"0\", \"on\", \"off\", \"yes\", \"no\", etc." << endl;
		cout << "" << endl;
		cout << "    -a alpha : force alpha channel to be on or off" << endl;
		cout << "             : potentially adds or removes the alpha layer" << endl;
		cout << "             : values are \"1\", \"0\", \"on\", \"off\", \"yes\", \"no\", etc." << endl;
		cout << "" << endl;
		cout << "    -O optimize : set the optimization status (default is off)" << endl;
		cout << "                : enabling optimization runs an optimization loop which may take longer" << endl;
		cout << "                : values are \"1\", \"0\", \"on\", \"off\", \"yes\", \"no\", etc." << endl;
		cout << "" << endl;
		cout << "    -S scatter : disperses data through the image more evenly" << endl;
		cout << "               : cancels out randomizer if enabled" << endl;
		cout << "               : values are \"1\", \"0\", \"on\", \"off\", \"yes\", \"no\", etc." << endl;
		cout << "" << endl;
		cout << "    -d downscale : if enabled, downscales the image to the minimum size needed" << endl;
		cout << "                 : can help out with reducing image size" << endl;
		cout << "                 : values are \"1\", \"0\", \"on\", \"off\", \"yes\", \"no\", etc." << endl;
		cout << "" << endl;
		cout << "    image.png : the file to embed data in" << endl;
		cout << "" << endl;
		cout << "    embed_file?.png : a list of files to store" << endl;
		cout << "" << endl;
		cout << "" << endl;
		cout << "    Also note, placing a \"--\" parameter causes any remaining parameters to be treated as files" << endl;

		return -1;
	}

	// PNG/JPG
	int pos = imageFile.length();
	bool isPng = imageFile.length() >= 4 && (imageFile[pos - 4] == '.' && (imageFile[pos - 3] & 0xDF) == 'P' && (imageFile[pos - 2] & 0xDF) == 'N' && (imageFile[pos - 1] & 0xDF) == 'G');

	// Load image
	cout << "Loading image..." << endl;
	std::vector<unsigned char> imageSrc;
	lodepng::load_file(imageSrc, imageFile.c_str());

	// Decode
	cout << "Decoding image..." << endl;
	Image image;
	stringstream errorStream;
	if (!image.loadFromSource(&imageSrc, isPng, channelCount, &errorStream)) {
		cout << "Error decoding image file \"" << imageFile << "\":" << endl;
		cout << "  " << errorStream.str() << endl;
		return -1;
	}
	channelCount = image.getChannelCount();

	// Image writer
	ImageWriter iw(&image);

	// Image info
	if (sources.size() == 0) {
		// Image stats
		cstring pad = "    ";
		cout << "Image stats for \"" << imageFile << "\":" << endl;
		for (unsigned int cc = 3; cc <= 4; ++cc) {
			cout << pad << "Width=" << image.getWidth() << "; Height=" << image.getHeight() <<
					"; Channels=" << cc;
			if (cc == image.getDefaultChannelCount()) cout << " (default)";
			cout << endl;

			cstring units;
			for (int i = 1; i <= 8; ++i) {
				double byteSpace = (iw.getBitAvailability(i, cc, 0, scatter, image.getWidth(), image.getHeight()) / 8);
				if (byteSpace >= 1024 * 1024) {
					byteSpace /= 1024 * 1024;
					units = "MB";
				}
				else if (byteSpace >= 1024) {
					byteSpace /= 1024;
					units = "KB";
				}
				else {
					units = "B";
				}
				cout << pad << "About " << std::fixed << std::setprecision(2) << byteSpace << units << " storable @ bitmask=" << i << endl;
			}
			cout << endl;
		}

		// Done
		return 0;
	}

	// Output filename
	if (outputFile.length() == 0) {
		outputFile = imageFile;
		int pos = outputFile.length();
		while (--pos >= 0 && outputFile[pos] != '.');
		if (pos < 0) pos = outputFile.length();

		outputFile.insert(pos, "-embed");

		if (!isPng) {
			// PNG extension required
			pos = outputFile.length();
			while (--pos >= 0 && outputFile[pos] != '.');
			if (pos < 0) pos = 0;
			outputFile.replace(pos, outputFile.length() - pos, ".png");
		}
	}

	// Size requirement
	unsigned int bitRequirement = iw.getBitRequirement(sources);

	// Filesize loop
	std::vector<unsigned char> pngOutput;
	int outputSize;
	int outputSizeMin = -1;
	bool filesizeLoop = true;
	int filesizeLoopIndex = -1;
	stringstream statusStream;
	while (filesizeLoop) {
		++filesizeLoopIndex;
		filesizeLoop = false;

		// Pass
		cout << endl << "Pass " << (filesizeLoopIndex + 1) << ":" << endl;

		// Reload/cache image?
		// Only performed if this loops
		if (filesizeLoopIndex > 0) {
			errorStream.str("");
			if (!image.loadFromSource(&imageSrc, isPng, channelCount, &errorStream)) {
				cout << "  Error: couldn't re-decode image file \"" << imageFile << "\":" << endl;
				cout << "    " << errorStream.str() << endl;
				return -1;
			}
		}

		// Bitmask
		if (bitmask == 0) {
			for (bitmask = 1; bitmask <= 8 && (bitRequirement > iw.getBitAvailability(bitmask, channelCount, 0, scatter, image.getWidth(), image.getHeight())); ++bitmask);
			if (bitmask > 8) {
				cout << "  Error: cannot embed files in the image given." << endl;
				cout << "    Max Available: " << iw.getBitAvailability(8, channelCount, 0, scatter, image.getWidth(), image.getHeight()) << " bytes" << endl;
				cout << "    Requires     : " << bitRequirement << " bytes" << endl;
				return -1;
			}
		}
		cout << "  Using bitmask of " << bitmask << " with " << channelCount << " channels" << endl;

		// Optimal size checking
		int dimensionsBest[2];
		dimensionsBest[0] = image.getWidth();
		dimensionsBest[1] = image.getHeight();
		// Crappy brute force method
		{
			int dimensions[2];
			int dimensionsTemp[2];
			dimensions[0] = image.getWidth();
			dimensions[1] = image.getHeight();
			bool i = (dimensions[1] > dimensions[0]);
			double scale = static_cast<double>(dimensions[!i]) / dimensions[i];

			for (int offset = 0; dimensions[i] - offset > 0; ++offset) {
				dimensionsTemp[i] = dimensions[i] - offset;
				dimensionsTemp[!i] = static_cast<int>(dimensions[!i] - offset * scale + 0.5); // 0.5 used for rounding

				if (bitRequirement > iw.getBitAvailability(bitmask, channelCount, 0, scatter, dimensionsTemp[0], dimensionsTemp[1])) {
					break;
				}

				dimensionsBest[0] = dimensionsTemp[0];
				dimensionsBest[1] = dimensionsTemp[1];
			}
		}
		if (static_cast<unsigned int>(dimensionsBest[0]) != image.getWidth() && static_cast<unsigned int>(dimensionsBest[1]) != image.getHeight()) {
			if (downscale) {
				cout << "  Downscaling..." << endl;
				image.downscale(dimensionsBest[0], dimensionsBest[1]);
				cout << "  Image downscaled to { " << dimensionsBest[0] << " x " << dimensionsBest[1] << " }" << endl;
			}
			else {
				cout << "  Optimal image dimensions for storage are { " << dimensionsBest[0] << " x " << dimensionsBest[1] << " }" << endl;
			}
		}

		// Packing
		cout << "  Packing..." << endl;
		int packCount = iw.pack(sources, bitmask, randomizeAll, scatter);
		if (packCount < 0) {
			cout << "  Error packing data into image" << endl;
		}

		// Writing
		cout << "  Encoding..." << endl;
		lodepng::State state;
		state.encoder.filter_palette_zero = 0;
		state.encoder.add_id = false;
		state.encoder.text_compression = 1;
		state.encoder.zlibsettings.nicematch = 258;
		state.encoder.zlibsettings.lazymatching = 1;
		state.encoder.zlibsettings.windowsize = 32768;
		state.encoder.zlibsettings.btype = 2;
		state.encoder.auto_convert = LAC_AUTO;
		if (fullOptimize) {
			statusStream.str("");
			errorStream.str("");
			if (!fullOptimizeEncode(&image, &state, &pngOutput, &std::cout, &statusStream, &errorStream)) {
				cout << "  Error: failed to encode" << endl;
				cout << "    " << errorStream.str();
				return -1;
			}
			cout << statusStream.str();
		}
		else {
			errorStream.str("");
			if (!image.saveToVector(&pngOutput, &state, &errorStream)) {
				cout << "  Error: failed to encode" << endl;
				cout << "    " << errorStream.str() << endl;
				return -1;
			}
		}

		// Filesize check
		outputSize = pngOutput.size();
		if (outputSize < outputSizeMin || outputSizeMin < 0) outputSizeMin = outputSize;
		if (filesizeLimit > 0) {
			if (outputSize > filesizeLimit) {
				// Re-loop
				cout << "  Warning: output file size limit exceeded" << endl;
				cout << "    Limit    : " << filesizeLimit << " bytes" << endl;
				cout << "    Generated: " << outputSize << " bytes" << endl;

				// Turn off randomization
				if (randomizeAll) {
					cout << "  Warning: changing randomization from \"on\" to \"off\"" << endl;
					randomizeAll = false;
				}
				else {
					if (!downscale) {
						downscale = true;
						cout << "  Warning: enabling downscaling" << endl;
					}
					else {
						// Turn off scatter
						if (scatter) {
							cout << "  Warning: changing scatter from \"on\" to \"off\"" << endl;
							scatter = false;
						}
						else {
							if (bitmask < 8) {
								cout << "  Warning: increasing bitmask from \"" << (bitmask) << "\" to \"" << (bitmask + 1) << "\"" << endl;
								bitmask += 1;
							}
							else {
								// Error
								cout << endl;
								cout << "  Error: output file size could not be resolved" << endl;
								cout << "    Limit  : " << filesizeLimit << " bytes" << endl;
								cout << "    Minimum: " << outputSizeMin << " bytes" << endl;
								cout << "  (note: to bypass the file size check, use \"-s 0\")" << endl;
								return -1;
							}
						}
					}
				}

				// Next
				filesizeLoop = true;
				continue;
			}
		}

		// Save
		cout << "  Saving..." << endl;
		lodepng::save_file(pngOutput, outputFile.c_str());
	}

	cout << endl << "Done" << endl;
	return 0;
}



bool arg2bool(cstring arg) {
	return (
		stricmp(arg, "on") == 0 ||
		stricmp(arg, "all") == 0 ||
		stricmp(arg, "yes") == 0 ||
		stricmp(arg, "true") == 0 ||
		stricmp(arg, "1") == 0 ||
		stricmp(arg, "enable") == 0 ||
		stricmp(arg, "enabled") == 0
	);
}



LodePNGFilterStrategy fullOptimizeEncodeStrategies[4] = { LFS_ZERO, LFS_MINSUM, LFS_ENTROPY, LFS_BRUTE_FORCE };
std::string fullOptimizeEncodeStrategynames[4] = { "LFS_ZERO", "LFS_MINSUM", "LFS_ENTROPY", "LFS_BRUTE_FORCE" };
int fullOptimizeEncodeMinmatches[2] = { 3, 6 };

bool fullOptimizeEncode(Image* image, lodepng::State* state, std::vector<unsigned char>* pngOutput, ostream* immediateStream, ostream* statusStream, ostream* errorStream) {
	// Threading this might be cool, but it isn't used very often so...
	assert(image != NULL);
	assert(state != NULL);
	assert(pngOutput != NULL);

	pngOutput->clear();

	size_t minSize = 0;
	bool first = true;

	int best[2] = { 0 , 0 };

	state->encoder.zlibsettings.btype = 2;
	state->encoder.auto_convert = LAC_AUTO;

	if (immediateStream != NULL) {
		*immediateStream << "  Detecting best encoding type..." << endl;
	}

	for (int i = 0; i < 4; i++) { // filter strategy
		for (int j = 0; j < 2; j++) { // min match
			std::vector<unsigned char> temp;
			state->encoder.filter_strategy = fullOptimizeEncodeStrategies[i];
			state->encoder.zlibsettings.minmatch = fullOptimizeEncodeMinmatches[j];
			int error = lodepng::encode(temp, *image->getPixels(), image->getWidth(), image->getHeight(), *state);

			if (error) {
				if (errorStream != NULL) {
					*errorStream << lodepng_error_text(error) << std::endl;
				}
				return false;
			}

			if (first || temp.size() < minSize) {
				minSize = temp.size();
				first = false;

				best[0] = i;
				best[1] = j;

				temp.swap(*pngOutput);
			}
		}
	}

	if (statusStream != NULL) {
		*statusStream << "    Filter strategy: " << fullOptimizeEncodeStrategynames[best[0]] << std::endl;
		*statusStream << "    Min match: " << fullOptimizeEncodeMinmatches[best[1]] << std::endl;
	}

	// Done
	return true;
}

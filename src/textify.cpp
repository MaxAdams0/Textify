#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_RESIZE2_IMPLEMENTATION

// This is temporary removals of flags for building
// Delete this after all bugs are squashed. Next step is performance!
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wfloat-conversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <windows.h>
#include <string>
#include <filesystem>
namespace fs = std::filesystem;
#include <chrono>
namespace sc = std::chrono;
#include <vector>
#include <thread>
#include <iostream>
#include <tuple>
#include <math.h>

#include "../lib/stb/stb_image.h"

#define OUTPUT_CHANNELS 3
const std::string charmap = " .-=+*#%@";

/* Get the Terminal window size in characters */
std::tuple<int, int> GetWindowSize();
/* Force the window to maximize (borderless fullscreen) */
void MaximizeWindow();
/* Get which file will be converted into text. Requires user input as an integer. */
std::string GetInputFile();
/* Simple Util to check if string is in vector. This might be a built in function */
bool IsStringInVec(std::string str, std::vector<std::string> vec);
/* Get the file extension from a file's name/path */
std::string GetFileExt(std::string fileName);
/* Check if the file's extension is valid (as in it is supported by this program & stb) */
int IsValidAndType(std::string fileExt);
/* Get the ascii-converted image from a file's path */
std::string GetImage(std::string filePath, int winWidth, int winHeight);
/* Calculate the luminescence (brightness) of a pixel */
int CalcLumine(int r, int g, int b);
/* Turn a value from an original range (0 onward) to a new range (0 onward) */
int Normalize(int val, int oldmax, int newmax);

int main()
{
	//std::ios::sync_with_stdio(false);
	if (!fs::exists("res"))
		if (!fs::create_directory("res"))
			perror("Error creating directory");

	MaximizeWindow();

	// Get which file to use for the image
	std::string fileName = GetInputFile();
	std::string filePath = fs::current_path().string() + "\\res\\" + fileName;
	std::cout << "Chosen file path: '" << filePath.c_str() << "'\n";

	// Varify that the file is readable (valid), and get its type (image/sequence)
	std::string fileExt = GetFileExt(filePath);
	int fileType = IsValidAndType(fileExt);
	// Do different actions for each type of input

	auto [winWidth, winHeight] = GetWindowSize();
	std::cout << "Window Size: " << winWidth << " x " << winHeight << "\n";
	if (fileType == -1) {
		std::cout << "The provided input type/extension is not supported, sorry!\n";
	} 
	else if (fileType == 0) {
		std::cout << "\r" << GetImage(filePath, winWidth, winHeight);
	}
	else if (fileType == 1) {
		// Get goal FPS/Frametime
		int fpsGoal;
		std::cout << "Set fps goal: ";
		std::cin >> fpsGoal;
		float frameTimeGoal = (1.0f / float(fpsGoal)) * 1000; // s -> ms
		std::cout << "Frame time goal: " << frameTimeGoal << "\n";
		// Read all elements in the dir
		std::vector<std::string> files = {};
		for (const auto & entry : fs::directory_iterator(filePath))
			files.emplace_back(entry.path().string());
		// Let the frame processing begin!
		std::vector<std::string> asciiFrames;
		int totalProcessingTime = 0;
		for (auto & file : files) {
			std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
			asciiFrames.emplace_back(GetImage(file, winWidth, winHeight));
			std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
			totalProcessingTime += std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
			// add progress bar at some point
			std::cout << "\rPath: '" << file << "' | Elapsed Time (ms): " << totalProcessingTime;
		}
		// Print each frame with a delay to sustain FPS goal
		// Warning: still can be perminantly behind when (frame time > 1/fps)
		system("cls");
		for (auto & frame : asciiFrames) {
			std::cout << "\r" << frame;
			// The frame time goal in seconds minus the frame print time in seconds
			// I don't know if subtracting the cout time makes sense, as doing the operation may take equal time
			float delay = frameTimeGoal;
			Sleep(delay);
		}

	} else {
		std::cout << "IsValidAndType gave erroneous response. Issue Unknown.\n";
	}

	std::cout << "Closing in 5 seconds...";
	Sleep(5000);
}

std::tuple<int, int> GetWindowSize()
{
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(hConsole, &csbi);
	return {csbi.srWindow.Right - csbi.srWindow.Left + 1, 
			csbi.srWindow.Bottom - csbi.srWindow.Top + 1};
}

void MaximizeWindow()
{
	HWND consoleWindow = GetConsoleWindow();
	ShowWindow(consoleWindow, SW_MAXIMIZE);
}

std::string GetInputFile()
{
	std::vector<std::string> files = {};

	for (const auto & entry : fs::directory_iterator("res\\")) {
		std::string entryPath = entry.path().string();
		entryPath = entryPath.substr(4); // remove "res\\"
		files.emplace_back(entryPath);
	}

	// display every file in the directory
	unsigned int i = 0;
	for (auto & filepath : files)
		std::cout << i++ << " " << filepath << "\n";


	// select one of the displayed files
	int selection;
	std::cout << "What file do you choose?: ";
	std::cin >> selection;

	std::string chosenFile = files.at(selection);

	return chosenFile;
}

bool IsStringInVec(std::string str, std::vector<std::string> vec)
{
	for (auto & elem : vec)
		if (elem == str)
			return true;
	
	return false;
}

std::string GetFileExt(std::string fileName)
{
	// get the index of the "." in the filename, and then set ext to everything past it
	int fileExtIndex = fileName.find(".");
	if (fileExtIndex == std::string::npos)
		return "";
	std::string fileExt = fileName.substr(fileExtIndex);
	return fileExt;
}

int IsValidAndType(std::string fileExt)
{
	std::vector<std::string> valid = {".jpg"};
	if (IsStringInVec(fileExt, valid))
		return 0;
	else if (fileExt.empty())
		return 1;
	else
		return -1;
}

std::string GetImage(std::string filePath, int winWidth, int winHeight)
{
	// Load the image file
	int imgWidth, imgHeight, channels;
	unsigned char *image = stbi_load(filePath.c_str(), &imgWidth, &imgHeight, &channels, OUTPUT_CHANNELS);
	if (image == nullptr) {
		perror("Error loading image.");
	}

	int newWidth = (float(imgWidth) / float(imgHeight)) * winHeight * 2;
	float widthRatio = float(imgWidth) / float(newWidth);
	float heightRatio = float(imgHeight) / float(winHeight);

	std::string frame;
	for (float y = 0; y < imgHeight; y += heightRatio) {
		for (float x = 0; x < imgWidth; x += widthRatio) {
			int pixelIndex = (int(y) * imgWidth + int(x)) * channels;
			int r = image[pixelIndex + 0];
			int g = image[pixelIndex + 1];
			int b = image[pixelIndex + 2];
			int lum = CalcLumine(r, g, b);
			lum = Normalize(lum, 255, charmap.size());
			frame += charmap[lum];
		}
		frame += "\n";
	}

	stbi_image_free(image);
	return frame;
}

int CalcLumine(int r, int g, int b){
	return int((r + g + b) / 3.0);
}

int Normalize(int value, int oldmax, int newmax) {
	return int(float(value) / float(oldmax) * newmax);
}

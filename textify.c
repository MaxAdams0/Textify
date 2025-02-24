#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <unistd.h>
#include <windows.h>
#include <time.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

struct iXY {
	int x;
	int y;
};
// Array sizes
#define MAX_CMD_SIZE 512
#define MAX_CMD_RETURN 16

// Commands (all are missing -i/-o)
#define FFPROBE_FPS_CMD "" // path at end (no -i)
#define FFPROBE_DUR_CMD "" // path at end (no -i)
// ffmpeg -i "./res/aot_37.mkv" -vf "scale=237:64,format=gray" -pix_fmt gray -vsync 0 -y "./res/frames/frame_%07d.bmp"
#define FFMPEG_GEN_FRAMES "" // requries -i & -o

// Image conversion 
#define CHARMAP " .,~-+<oiOIPBA#@" // " `.-':_,^=;><+!rc*/z?sLTv)J7(|Fi{C}fI31tlu[neoZ5Yxjya]2ESwqkP6h9d4VpOGbUAKXHm8RD#$Bg0MNWQ%&@"
#define CALC_LUMINESCENCE(r,g,b) (int)(0.299 * r + 0.587 * g + 0.114 * b);

// Win32
void MaximizeConsoleWindow();
void GetConsoleSize(int *rows, int *cols);
void ClearScreen();
void HideCursor();
int GetDirFileCount(const char *directory);
char** ListDir(const char *dir, int *fileCount);
void FreeFilePaths(char **filePaths, int fileCount);

// Math
int Norm(int v, int omax, int nmax);

// Other
char* PathConcat(const char* p1, const char* p2);
void RunCommand(char* output, const char *command, const int lineSize);
void FillCommands(char* fpsCmd, char* durCmd, char* genCmd);


bool ARG_DEBUG = false;

int main(int argc, char *argv[]) {
	for (int i=0; i < argc; i++) {
		if (strcmp(argv[i], "-debug") == 0) ARG_DEBUG = true;
	}
	
	// Setup / get console size
	MaximizeConsoleWindow();
	struct iXY winSize;
	GetConsoleSize(&winSize.x, &winSize.y);
	printf("Window Size: %d x %d\n", winSize.x, winSize.y);
	
	// Get paths
	char homeDir[MAX_PATH], frameDir[MAX_PATH], videoPath[MAX_PATH];
	getcwd(homeDir, sizeof(homeDir));
	snprintf(frameDir, sizeof(frameDir), "%s\\temp_frames", homeDir);
	printf("Input video file path: ");  
	scanf("%s", videoPath);
	
	// Fill commands
	char fpsCmd[MAX_CMD_SIZE], durCmd[MAX_CMD_SIZE], genCmd[MAX_CMD_SIZE];
	snprintf(fpsCmd, sizeof(fpsCmd), "ffprobe -v quiet -select_streams v -of default=noprint_wrappers=1:nokey=1 -show_entries stream=r_frame_rate \"%s\"", videoPath);
	snprintf(durCmd, sizeof(durCmd), "ffprobe -v quiet -of csv=p=0 -show_entries format=duration \"%s\"", videoPath);
	snprintf(genCmd, sizeof(genCmd), "ffmpeg -v quiet -i \"%s\" -vf scale=237:64,format=gray -pix_fmt gray -vsync 0 -y \"%s\\frame_%%07d.bmp\"", videoPath, frameDir);
	
	// Run commands
	char fpsRes[MAX_CMD_RETURN], durRes[MAX_CMD_RETURN], genRes[MAX_CMD_RETURN];
	puts("Retrieving video fps...");
	if (ARG_DEBUG) puts(fpsCmd); // DEBUG
	RunCommand(fpsRes, fpsCmd, winSize.x);
	if (ARG_DEBUG) puts(fpsRes); // DEBUG
	
	puts("Retrieving video duration...");
	if (ARG_DEBUG) puts(durCmd); // DEBUG
	RunCommand(durRes, durCmd, winSize.x);
	if (ARG_DEBUG) puts(durRes); // DEBUG
	
	puts("Converting video into frames...");
	if (ARG_DEBUG) puts(genCmd); // DEBUG
	RunCommand(genRes, genCmd, winSize.x);
	if (ARG_DEBUG) puts(genRes); // DEBUG
	
	// Process command outputs
	float v_fps, v_dur;
	
	float num, denom;
	if (sscanf(fpsRes, "%f/%f", &num, &denom) != 2) {
		puts("Error processing fps ouput.");
		return EXIT_FAILURE;
	}
	v_fps = num / denom;
	v_dur = atof(durRes);
	
	float frameTime = (1.0 / v_fps) * 1000.0; // s -> ms
	int frameCount = GetDirFileCount(frameDir);
	if (frameCount <= 0) {
		printf("Could not find any files in directory '%s'", frameDir);
		return -1;
	}
	
	printf("FPS: %05f, FT: %05fms, Frames: %d, Duration: %05fs\n", v_fps, frameTime, frameCount, v_dur);
	
	char **filePaths = ListDir(frameDir, &frameCount);
	if (filePaths == NULL) {
		return -1;
	}
	
	puts("Video will begin in 3 seconds...");
	Sleep(3000);
	ClearScreen();
	HideCursor();
	
	clock_t start, end;
	double cpuTimeLapsed;
	double waitTime = 0; // so output matches same fps as video
	for (int i = 0; i < frameCount; i++) {
		start = clock();
		
		int chan = 0;
		unsigned char *img = stbi_load(filePaths[i], &winSize.x, &winSize.y, &chan, 3);
		if (img == NULL){
			perror("Error loading image.");
			return -1;
		}
		
		char ascii[winSize.x * winSize.y];
		for (int y=0; y < winSize.y; y++) {
			for (int x=0; x < winSize.x; x++) {
				int pIndex = chan * (y * winSize.x + x);
				
				int r = img[pIndex + 0];
				int g = img[pIndex + 1];
				int b = img[pIndex + 2];
				int lum = CALC_LUMINESCENCE(r, g, b);
				
				lum = Norm(lum, 255, strlen(CHARMAP)-1);
				char c = CHARMAP[lum];
				ascii[y * winSize.x + x] = c;
			}
			ascii[y * winSize.x + winSize.x-1] = '\n';
		}
		ClearScreen();
		ascii[(winSize.y-1) * winSize.x + (winSize.x-1)] = '\0';
		fputs(ascii, stdout);
		fflush(stdout);
		
		end = clock();
		cpuTimeLapsed = ((double)(end - start)) / CLOCKS_PER_SEC; // ms / milliseconds
		waitTime = (frameTime - cpuTimeLapsed) * 1000.0; // microseconds
		
		stbi_image_free(img);
		usleep(waitTime);
	}


	// Free the dynamically allocated memory
	FreeFilePaths(filePaths, frameCount);
	return 0;
}

// Win32

void MaximizeConsoleWindow() {
	HWND hwnd = GetConsoleWindow();
	if (hwnd) {
		ShowWindow(hwnd, SW_MAXIMIZE);
	} else {
		puts("Unable to maximize window (could not get hwnd)");
	}
}

void GetConsoleSize(int *rows, int *cols) {
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
		*cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
		*rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
	} else { // failure case 
		*cols = 0;
		*rows = 0;
	}
}

// ***Gives the console a behavior of 'overwriting' the screen instead of 'line-by-line' writing
void ClearScreen() {
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(hConsole, &csbi);

	// set cursor to top left
	COORD coord = {0, 0};
	SetConsoleCursorPosition(hConsole, coord);
}

// Prevent cursor flashing blocking image
void HideCursor() {
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_CURSOR_INFO cursorInfo;
	GetConsoleCursorInfo(hConsole, &cursorInfo);
	cursorInfo.bVisible = FALSE;  // Hide the cursor
	SetConsoleCursorInfo(hConsole, &cursorInfo);
}

int GetDirFileCount(const char *dir) {
	WIN32_FIND_DATA data;
	HANDLE handle;
	int count = 0;

	char searchPath[MAX_PATH];
	snprintf(searchPath, sizeof(searchPath), "%s\\*", dir);

	handle = FindFirstFile(searchPath, &data);
	if (handle == INVALID_HANDLE_VALUE) {
		printf("Error opening directory: %s\n", dir);
		return -1;
	}

	do {
		if (!(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			count++;
		}
	} while (FindNextFile(handle, &data) != 0);

	FindClose(handle);
	return count;
}

char** ListDir(const char *dir, int *fileCount) {
	WIN32_FIND_DATA fData;
	HANDLE fHandle;

	char searchPath[MAX_PATH];
	snprintf(searchPath, sizeof(searchPath), "%s\\*", dir);

	fHandle = FindFirstFile(searchPath, &fData);
	if (fHandle == INVALID_HANDLE_VALUE) {
		printf("Error opening dir: '%s'\n", dir);
		printf("GetLastError() = %lu\n", GetLastError());
		return NULL;
	}

	int count = 0;
	// Allocate initial memory for file paths
	int capacity = 10;
	char **filePaths = (char **)malloc(capacity * sizeof(char *));
	if (filePaths == NULL) {
		printf("Memory allocation failed.\n");
		FindClose(fHandle);
		return NULL;
	}

	do {
		if (!(fData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			// Ensure there's enough space in the array
			if (count == capacity) {
				capacity *= 2;
				filePaths = (char **)realloc(filePaths, capacity * sizeof(char *));
				if (filePaths == NULL) {
					printf("Memory allocation failed.\n");
					FindClose(fHandle);
					return NULL;
				}
			}

			// Allocate memory for each file path and copy the name
			filePaths[count] = (char *)malloc(MAX_PATH * sizeof(char));
			if (filePaths[count] == NULL) {
				printf("Memory allocation failed.\n");
				FindClose(fHandle);
				return NULL;
			}

			snprintf(filePaths[count], MAX_PATH, "%s\\%s", dir, fData.cFileName);
			count++;
		}
	} while (FindNextFile(fHandle, &fData) != 0);

	FindClose(fHandle);

	*fileCount = count;
	return filePaths;
}

void FreeFilePaths(char **filePaths, int fileCount) {
	for (int i = 0; i < fileCount; i++) {
		free(filePaths[i]);
	}
	free(filePaths);
}

// Math

int Norm(int v, int omax, int nmax) {
	int weight = omax / nmax;
	return v / weight;
}

// Other

char* PathConcat(const char* p1, const char* p2) {
	static char buffer[MAX_PATH];
	snprintf(buffer, sizeof(buffer), "%s%s", p1, p2);
	return buffer;
}

void RunCommand(char* output, const char *command, const int lineSize) {
	FILE *fp;
	char buffer[lineSize];

	fp = popen(command, "r"); // treating console as file
	if (fp == NULL) {
		perror("popen failed");
		return;
	}

	// read output line-by-line
	while (fgets(buffer, sizeof(buffer), fp) != NULL) {
		strcat(output, buffer);
	}

	pclose(fp);
}


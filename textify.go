package main

import (
	"fmt"
	//"time"
	"golang.org/x/image/bmp"
	"log"
	"os"
	"os/exec"
	"strings"
	"strconv"

	"github.com/inancgumus/screen"
)

func main() {
	wW, wH := screen.Size()
	fmt.Printf("Window Size: %d x %d\n", wW, wH)

	// Get the input file's path
	pwd, err := os.Getwd()
	ReportError(err)
	inputPath := pwd + "\\res\\" + GetInputMedia()
	//TEMP_FRAME_NAME := pwd + "\\_temp_.bmp"
	fmt.Printf("Chosen File: '%s'\n", inputPath)
	
	// Get goal FPS/Frametime
	fpsGoal, err := FFprobeGetFps(inputPath)
	ReportError(err)
	duration, err := FFprobeGetDuration(inputPath)
	ReportError(err)
	
	frameCount := int(duration * fpsGoal)
	
	frameTimeGoal := float32(1) / float32(fpsGoal)
	fmt.Printf("Fps: %05f, Duration: %05f, Frame Count: %d\n", fpsGoal, duration, frameCount)
	fmt.Printf("Frame Time Goal: %05f\n", frameTimeGoal)
	
	// prerun
	files, err := os.ReadDir(pwd + "\\res\\frames")
	ReportError(err)
	
	i := 0
	var asciiFrames []string
	for _, f := range files {
		framePath := pwd + "\\res\\frames" + "\\" + f.Name()
		
		asciiFrames = append(asciiFrames, ImageToAscii(framePath))
		
		fmt.Printf("\r%d/%d", i, frameCount)
		i++
		if i == 2000 {
			break
		}
	}
	
	for i := range frameCount {
		fmt.Printf("\r%s", asciiFrames[i])
	}
}

// ==================== APPLICATION SPECIFIC FUNCTIONS ====================================================================

func GetInputMedia() string {
	// Read all elements in the dir "res"
	files, err := os.ReadDir("res")
	ReportError(err)
	// Print every element
	for i, f := range files {
		fmt.Printf("%d %s\n", i, f.Name())
	}
	// Get file selection from integer input
	fmt.Print("Choose file (#): ")
	var chosenFile int
	_, err = fmt.Scanf("%d", &chosenFile)
	ReportError(err)
	if chosenFile >= len(files) {
		log.Fatal("Selected file does not exist. Number provided is likely too large or small.")
	}
	// Return the name of the selection
	return files[chosenFile].Name()
}

// =========================================================== FFMPEG =====================================================

func FFprobeTranslateDurationResult(input string) (float32, error) {
	input = strings.TrimSpace(input) // jik
	// if not removed, atoi will return 0
	input = strings.Trim(input, "\n") // Remove newline if any
	
	duration, err := strconv.ParseFloat(input, 64)
	if err != nil {
		return 0.0, err
	}
	
	return float32(duration), nil
}

func FFprobeTranslateFpsResult(input string) (float32, error) {
	input = strings.TrimSpace(input) // jik
	// if not removed, atoi will return 0
	input = strings.Trim(input, "\n") // Remove newline if any

	parts := strings.Split(input, "/")
	if len(parts) != 2 {
		return 0, fmt.Errorf("invalid format: %s", input)
	}

	num, err1 := strconv.Atoi(parts[0])
	denom, err2 := strconv.Atoi(parts[1])

	if err1 != nil || err2 != nil || denom == 0 {
		fmt.Printf("\n")
		return 0, fmt.Errorf("invalid numbers or division by zero: %s", input)
	}

	return float32(num) / float32(denom), nil
}

func FFprobeGetFps(videoPath string) (float32, error) {
	cmd := exec.Command("ffprobe", "-v", "error", "-select_streams", "v", "-of",
		"default=noprint_wrappers=1:nokey=1", "-show_entries", "stream=r_frame_rate", videoPath)

	output, err := cmd.Output()
	if err != nil {
		return 0.0, err
	}
	output_str := string(output)

	fps, err := FFprobeTranslateFpsResult(output_str)
	if err != nil {
		return 0.0, err
	}

	return fps, nil
}

func FFprobeGetDuration(videoPath string) (float32, error) {
	cmd := exec.Command("ffprobe", "-show_entries", "format=duration", "-v", "quiet", "-of", "csv=p=0", videoPath)

	output, err := cmd.Output()
	if err != nil {
		return 0.0, err
	}
	output_str := string(output)
	
	duration, err := FFprobeTranslateDurationResult(output_str)
	if err != nil {
		return 0.0, err
	}
	
	return float32(duration), nil
}

// ========================================================== DISPLAY =====================================================

func FFmpegSaveFrame(videoPath, imagePath string, frameIndex int, fps float32) (error) {
	time := float32(frameIndex) / fps
	timeStr := fmt.Sprintf("%0.5f", time)
	wW, wH := screen.Size()
	resizeStr := fmt.Sprintf("scale=%d:%d", wW-1, wH-1)
	
	// (EX) ffmpeg -ss 100.00 -i "./res/LuckyStar1.mp4" -vframes 1 -q:v 2 -y -v quiet -update 1 -output.bmp
	// '-ss' 		makes it only process the video at the time stamp
	// '-y' 		makes it auto answer yes (this happens when you try to overwrite a file)(although -update fixes this)
	// '-v quiet' 	makes the output not be printed
	cmd := exec.Command("ffmpeg", "-ss", timeStr, "-i", videoPath, "-vframes", "1", 
		"-q:v", "2", "-y", "-v", "quiet", "-update", "1", "-vf", resizeStr, imagePath)
		
	_, err := cmd.Output()
	if err != nil {
		return err
	}
	
	return nil
}

// TODO: OPTIMIZE THE SH*T OUT OF THIS
func ImageToAscii(filePath string) string {
	charmap := " .-=+*#%@" //[]rune{' ', '\u2591', '\u2592', '\u2593'}
	
	// Open the image
	file, err := os.Open(filePath)
	if err != nil {
		ReportError(err)
		return ""
		file.Close()
	}
	
	// Load the image into readable form
	img, err := bmp.Decode(file)
	ReportError(err)
	file.Close()
	
	// Get the image's size (bounds)
	bounds := img.Bounds()
	w, h := bounds.Max.X, bounds.Max.Y
	
	// Loop through every pixel and print the associated char with its luminescence
	var frame string
	for y := 0; y < h-3; y++ {
		for x := 0; x < w-3; x++ {
			r, _, _, _ := img.At(x, y).RGBA()
			lum := Normalize(int(r), 65535, len(charmap)-1)
			frame += string(charmap[lum])
		}
		frame += "\n"
	}
	return frame
}

// ==================== EXTRA UTILITY FUNCTIONS ===========================================================================

func ReportError(e error) {
	if e != nil {
		fmt.Printf("Error: %v\n", e)
	}
	return
}

func CalcLumine(r, g, b uint32) int {
	return int((r + g + b) / 3)
}

func Normalize(value, oldMax, newMax int) int {
	weight := oldMax / newMax
	return value / weight
}

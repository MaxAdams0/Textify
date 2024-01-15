# Textify
Hi! This is just a personal project to convert visual media into ASCII art, written in both C and Go! <br>
This project is something I come back to every once and a while, originally having written it in C++, but deciding that it is not the right language for me. Also, my old rusty code needed to be revitalized with new knowledge, so I decided to start over completely in C! Then I got tired of C's required boilerplate for basic features, and pulled a full 180 and re-wrote it all in Go!

## Future Update Goals:
- Video conversion/placyback*
- Terminal Color (option)
- Linux support
I would like to add more, of course, but I tend to be a little to ambitious with my projects. Even these are a little steep, so I guess we'll have to see how much gets done!

## Recources
From [STB](https://github.com/nothings/stb),
- [stb_image.h](https://github.com/nothings/stb/blob/master/stb_image.h)
- [stb_image_resize2.h](https://github.com/nothings/stb/blob/master/stb_image_resize2.h)

STB is held under the MIT Liscence: <br>
[STB's Licenses](https://github.com/nothings/stb/blob/master/LICENSE) <br>

### Extra Notes
*Unfortunately, this will most likely not be done as expected. Instead, it will be done using a sequence of images in a folder. Video encoding is too difficult, and there is really no solution around it, without then adding either too many libraries, too many errors, or too many limitations.

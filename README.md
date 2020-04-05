# tech_task

gcc code
```
gcc  gcc tech_task.cpp bmpHeader.cpp yuvVideo.cpp -o tech_task.exe -lavcodec -lavformat -lavfilter -lswscale -lavutil -lz -lstdc++
-I(ffmpeg headers)
```
For successful compilation, you must specify the path to the ffmpeg headers `-I(ffmpeg header)`

Console arguments to run program
```
tech_task fileBMP source_videofile_YUV420 output_videofile_YUV420 resolution_source_video
```
Example
```
tech_task test.bmp akiyo_cif.yuv outputFile.yuv 352x288
```

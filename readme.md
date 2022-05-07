##### 封装srt(未完成)

[srt url](https://github.com/Haivision/srt/blob/master/docs/features/access-control.md)  

ffmpeg推流
```
ffmpeg -offset_x 657 -offset_y 253 -video_size 800x600 -framerate 10 -f gdigrab -i desktop -qscale 0.01 -pix_fmt yuv420p -codec h264 -profile:v Baseline -f mpegts "srt://127.0.0.1:8855?streamid=uplive/live/test,m=publish"
```

ffplay播放
```
ffplay -i srt://127.0.0.1:8855?streamid=uplive/live/test
```

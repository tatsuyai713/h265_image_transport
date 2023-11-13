## h265_image_transport ![build status](https://github.com/clydemcqueen/h265_image_transport/actions/workflows/build_test.yml/badge.svg?branch=master)

Adds H265 decoding to [ROS2 image transport](https://index.ros.org/p/image_transport/github-ros-perception-image_common/).
Also includes a simple node to copy H265 packets from a `video4linux` camera.

Possible future work: add H265 encoder
 
### Example usage

Command line:
~~~
ros2 run image_transport republish h265 raw --ros-args -r in/h265:=/image_raw/h265 -r out:=/repub_raw
~~~

Launch file:
~~~
ros2 launch h265_image_transport example_launch.py
~~~

### h265_cam_node parameters

| Parameter | Type | Default | Notes |
|---|---|---|---|
| input_fn | string | /dev/video2 | Can be any ffmpeg format |
| fps | string | 30 | Desired frame rate |
| size | string | 800x600 | Width by height |
| frame_id | string | camera_frame | Camera frame ID |
| camera_info_path | string | info.ini | Path to camera info file |

### h265_cam_node topics

| Topic | Message | Notes |
|---|---|---|
| image_raw/h265 | `h265_image_transport::msg::H265Packet` | H265 packet |
| camera_info | `sensor_msgs::msg::CameraInfo` | Camera info message |

### Requirements

Tested on ROS2 Eloquent (Ubuntu 18.04) and Foxy (Ubuntu 20.04).

Eloquent libraries:
* libavdevice>=57.10.100
* libavformat>=57.83.100
* libavcodec>=57.107.100
* libavutil>=55.78.100
* libswscale>=4.8.100

Foxy libraries:
* libavdevice>=58
* libavformat>=58
* libavcodec>=58
* libavutil>=56
* libswscale>=5

Here's one way to satisfy these requirements:
~~~
sudo apt install libavdevice-dev libavformat-dev libavcodec-dev libavutil-dev libswscale-dev
~~~

Note: `rosdep` won't find keys for most of these libraries so `package.xml` declares
dependencies on `ffmpeg` and `libavdevice-dev`. Strictly speaking `ffmpeg` is not required.
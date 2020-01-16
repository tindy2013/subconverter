# subconverter-docker
------
This is a minimized image to run https://github.com/tindy2013/subconverter.

For running this docker, simply use the following commands:
```bash
# run the container detached, forward internal port 25500 to host port 25500
docker run -d -p 25500:25500 tindy2013/subconverter:latest
# then check its status
curl http://localhost:25500
# if you see `subconverter vx.x.x backend` then the container is up and running
```

If you want to update `pref.ini` inside the docker, you can use the following command:
```bash
# assume your configuration file name is `newpref.ini`
curl -F "@data=newpref.ini" http://localhost:25500/updateconf?type=form&token=password
# you may want to change this token in your configuration file
```

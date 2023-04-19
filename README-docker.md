# subconverter-docker

This is a minimized image to run https://github.com/tindy2013/subconverter.

For running this docker, simply use the following commands:
```bash
# run the container detached, forward internal port 25500 to host port 25500
docker run -d --restart=always -p 25500:25500 tindy2013/subconverter:latest
# then check its status
curl http://localhost:25500/version
# if you see `subconverter vx.x.x backend` then the container is up and running
```
Or run in docker-compose:
```yaml
---
version: '3'
services:
  subconverter:
    image: tindy2013/subconverter:latest
    container_name: subconverter
    ports:
      - "15051:25500"
    restart: always
```

If you want to update `pref` configuration inside the docker, you can use the following command:
```bash
# assume your configuration file name is `newpref.ini`
curl -F "data=@newpref.ini" http://localhost:25500/updateconf?type=form\&token=password
# you may want to change this token in your configuration file
```

For those who want to use their own `pref` configuration and/or rules, snippets, profiles:
```txt
# you can save the files you want to replace to a folder, then copy it into to the docker
# using the latest build of the official docker
FROM tindy2013/subconverter:latest
# assume your files are inside replacements/
# subconverter folder is located in /base/, which has the same structure as the base/ folder in the repository
COPY replacements/ /base/
# expose internal port
EXPOSE 25500
# notice that you still need to use '-p 25500:25500' when starting the docker to forward this port
```
Save the content above to a `Dockerfile`, then run:
```bash
# build with this Dockerfile and tag it subconverter-custom
docker build -t subconverter-custom:latest .
# run the docker detached, forward internal port 25500 to host port 25500
docker run -d --restart=always -p 25500:25500 subconverter-custom:latest
# then check its status
curl http://localhost:25500/version
# if you see `subconverter vx.x.x backend` then the container is up and running
```

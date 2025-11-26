DOCKER BUILD
==================
How to build Soteria binaries inside Docker containers and how to export them to the host PC.

Overview
---------------------
- Install a recent Docker Engine.
- Clone the Soteria source locally so the Docker build context includes the repository files.
- The Dockerfiles live under `doc/docker/` and currently target Linux (`Dockerfile-Linux64-bin`) and Windows (`Dockerfile-Win64-bin`) artifacts.
- The images compile Soteria inside `/root/Soteria` and leave the build products in that directory. The container keeps running (`tail -f /dev/null`) so you can copy files out after the build completes.

Building Linux x86_64 binaries
---------------------
1. From the repository root, build the image:
   ```bash
docker build -f doc/docker/Dockerfile-Linux64-bin -t soteria-linux64-build .
```
2. Create a stopped container from that image:
   ```bash
docker create --name soteria-linux64-out soteria-linux64-build
```
3. Copy the artifacts to the host. The most common binaries live in `/root/Soteria/src/`:
   ```bash
mkdir -p build-out/linux64
# Core daemons and CLI tools
docker cp soteria-linux64-out:/root/Soteria/src/soteriad build-out/linux64/
docker cp soteria-linux64-out:/root/Soteria/src/soteria-cli build-out/linux64/
docker cp soteria-linux64-out:/root/Soteria/src/soteria-tx build-out/linux64/
# Optional GUI wallet (built when Qt dependencies succeed)
docker cp soteria-linux64-out:/root/Soteria/src/qt/soteria-qt build-out/linux64/ || true
```
   If you prefer to archive everything, copy the full directory instead:
   ```bash
docker cp soteria-linux64-out:/root/Soteria/src/. build-out/linux64/
```
4. (Optional) Collect the pre-built dependency tree (useful for redistributable packages):
   ```bash
docker cp soteria-linux64-out:/root/Soteria/depends/x86_64-pc-linux-gnu/. build-out/linux64/depends/
```
5. Remove the temporary container when finished:
   ```bash
docker rm soteria-linux64-out
```

Building Windows x86_64 binaries
---------------------
1. Build the cross-compilation image:
   ```bash
docker build -f doc/docker/Dockerfile-Win64-bin -t soteria-win64-build .
```
2. Create a container from the image:
   ```bash
docker create --name soteria-win64-out soteria-win64-build
```
3. Copy the Windows artifacts to the host. Executables are placed under `/root/Soteria/src/`:
   ```bash
mkdir -p build-out/win64
# Daemon and command-line tools
docker cp soteria-win64-out:/root/Soteria/src/soteriad.exe build-out/win64/ || true
docker cp soteria-win64-out:/root/Soteria/src/soteria-cli.exe build-out/win64/ || true
docker cp soteria-win64-out:/root/Soteria/src/soteria-tx.exe build-out/win64/ || true
# GUI wallet (if Qt build succeeded)
docker cp soteria-win64-out:/root/Soteria/src/qt/soteria-qt.exe build-out/win64/ || true
# Copy the entire directory if you plan to package everything
docker cp soteria-win64-out:/root/Soteria/src/. build-out/win64/
```
4. Grab the cross-compiled dependency outputs if needed:
   ```bash
docker cp soteria-win64-out:/root/Soteria/depends/x86_64-w64-mingw32/. build-out/win64/depends/
```
5. Remove the container after extracting the files:
   ```bash
docker rm soteria-win64-out
```

Inspecting or customizing builds
---------------------
- Start an interactive shell to inspect the workspace:
  ```bash
docker run --rm -it soteria-linux64-build /bin/bash
```
  From there you can rerun `make`, adjust `-j` flags, or tweak configuration options.
- To rebuild with local source modifications, edit the repository and re-run `docker build`. Docker’s layer cache will reuse previously completed steps when possible.
- Adjust the Dockerfiles if you need different configure flags (for example, enabling tests or indexing).

Extracting artifacts with running containers
---------------------
If you prefer to keep the container running while copying files, replace the `docker create` step with:
```bash
docker run -d --name soteria-linux64-out soteria-linux64-build
docker cp soteria-linux64-out:/root/Soteria/src/soteriad build-out/linux64/
...
docker stop soteria-linux64-out && docker rm soteria-linux64-out
```

Cleanup
---------------------
- Remove build images when finished:
  ```bash
docker rmi soteria-linux64-build soteria-win64-build
```
- Delete the `build-out/` directory once artifacts are archived.

Troubleshooting
---------------------
- Use `docker logs <container>` to review the build output if a layer fails.
- Ensure the host has enough RAM and disk space; the images compile the full dependency tree, which can take upto 7-9 gigabytes depends on OS.
- If Docker is running on Windows or macOS, make sure file sharing is enabled for the drive that hosts the repository, otherwise the `docker build` context may be empty.

With these steps you can produce reproducible Linux and Windows binaries without installing the complete toolchain on your host.

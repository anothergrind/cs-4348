# Programming Project 2

## CS 4348: Operating Systems

Professor Mingming Chen

Kamsi Ozorji and Khoa Bui

### Testing on Linux (Docker)

```bash
cd project-2
docker build -t cs4348 .
docker run --rm -it -v "$(pwd)":/app cs4348
make && ./my-dining 5
```

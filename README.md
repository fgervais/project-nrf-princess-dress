# Project Management

## Init

```bash
mkdir project-nrf-princess-dress
cd project-nrf-princess-dress
docker run --rm -it -u $(id -u):$(id -g) -v $(pwd):/workdir/project nordicplayground/nrfconnect-sdk:v2.1-branch bash
west init -m https://github.com/fgervais/project-nrf-princess-dress.git .
west update
```

## Build

```bash
cd application
docker compose run --rm nrf west build -b pink_panda -s app
```

## menuconfig

```bash
cd application
docker compose run --rm nrf west build -b pink_panda -s app -t menuconfig
```

## Clean

```bash
cd application
rm -rf build/
```

## Flash

### nrfjprog
```bash
cd application
docker compose -f docker-compose.yml -f docker-compose.device.yml \
        run --rm nrf west flash
```

### pyocd
```bash
cd application
pyocd flash -e sector -t nrf52840 -f 4000000 build/zephyr/zephyr.hex
```

# Hardware

https://github.com/fgervais/project-nrf-princess-dress_hardware

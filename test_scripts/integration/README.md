# Integration tests (require live servers)

These verify the MariaDB and Memcached modules against real servers, so they are NOT
part of the globbed ctest suite (which must run unattended). Run them by hand after
starting the servers with Docker:

```bash
docker run -d --name vs-mariadb  -e MARIADB_ROOT_PASSWORD=testpw -e MARIADB_DATABASE=testdb -p 13306:3306 mariadb:11
docker run -d --name vs-memcached -p 21211:11211 memcached:1.6

voidscript "$PWD/test_scripts/integration/mariadb_connection.vs"
voidscript "$PWD/test_scripts/integration/memcached_connection.vs"
```

Each asserts the object-identity fix: a second connection is independent of the first
(disconnecting one leaves the other connected), which a shared-client collision would
break. They also exercise host:port parsing (verified against non-default ports 13306 /
21211).

## StableDiffusion (GPU + model + built module)

`stablediffusion_txt2img.vs` needs the StableDiffusion module built
(BUILD_MODULE_STABLEDIFFUSION=ON, see Modules/StableDiffusion/README.md), a GPU, and an
SD1.5 checkpoint. Verified against v1-5-pruned-emaonly-fp16.safetensors on an RTX 3060.

```bash
voidscript "$PWD/test_scripts/integration/stablediffusion_txt2img.vs"
```

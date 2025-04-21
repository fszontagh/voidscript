# FastCGI Integration for VoidScript

This document covers building the `voidscript-fcgi` FastCGI executable and configuring it under Apache2 and Nginx.

## Template Tag Syntax
`voidscript-fcgi` runs your `.vs` files as HTML-like templates: code is only executed inside the configured parser tags, and everything else is emitted verbatim.  By default, the open/close tags come from `options.h`:
```c
#define PARSER_OPEN_TAG  "<?void"
#define PARSER_CLOSE_TAG "?>"
```
Usage example in a `.vs` file:
```html
<html>
<body>
  <!-- outside tags: printed verbatim -->
  <?void
     print("Hello from VoidScript!");
  ?>
</body>
</html>
```

## Build Requirements
- CMake (>= 3.20)
- C++20-capable compiler (e.g. g++ 9+)
- libfcgi development headers (e.g. `libfcgi-dev` on Debian/Ubuntu)
- (Optional) `spawn-fcgi` for Nginx setups

### Building the FastCGI Binary
```bash
mkdir -p build && cd build
cmake .. -DBUILD_FASTCGI=ON
cmake --build . --target voidscript-fcgi
sudo cmake --install . --component bin
```

The `voidscript-fcgi` binary will be installed into your CMake install prefix (default `/usr/local/bin`).

## Apache2 Configuration
1. Install and enable Apache’s FastCGI module:
   ```bash
   sudo apt-get update
   sudo apt-get install libapache2-mod-fcgid
   sudo a2enmod fcgid
   ```
2. Ensure `voidscript-fcgi` is in your `$PATH` (e.g. `/usr/local/bin`).
3. Create a config snippet (e.g. `/etc/apache2/conf-available/voidscript-fcgi.conf`):
   ```apache
   <IfModule fcgid_module>
     <Directory "/var/www/html">
       Options +ExecCGI
       AddHandler fcgid-script .vs
       FcgidWrapper /usr/local/bin/voidscript-fcgi .vs
     </Directory>
   </IfModule>
   ```
4. Enable and reload Apache:
   ```bash
   sudo a2enconf voidscript-fcgi
   sudo systemctl reload apache2
   ```

Now any `.vs` script under your DocumentRoot will be handled by FastCGI.

## Nginx Configuration
Nginx does not manage FastCGI processes itself. A common approach is to spawn the FastCGI executable with `spawn-fcgi`:

```bash
sudo apt-get install spawn-fcgi
spawn-fcgi -s /var/run/voidscript-fcgi.sock -M 766 /usr/local/bin/voidscript-fcgi
```

Then add to your Nginx site (e.g. `/etc/nginx/sites-enabled/default`):
```nginx
server {
    listen 80;
    server_name example.com;
    root /var/www/html;

    location ~ \.vs$ {
        include fastcgi_params;
        fastcgi_pass unix:/var/run/voidscript-fcgi.sock;
        fastcgi_param SCRIPT_FILENAME $document_root$fastcgi_script_name;
    }
}
```

Reload Nginx:
```bash
sudo systemctl reload nginx
```

Your `.vs` scripts will now execute via FastCGI under Nginx.
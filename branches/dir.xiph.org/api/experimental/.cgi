#!/bin/bash
echo Content-Type: text/plain
echo 

ls -1 /var/www/api.dir.xiph.org/experimental/ | sed -e 's/\..*$//'

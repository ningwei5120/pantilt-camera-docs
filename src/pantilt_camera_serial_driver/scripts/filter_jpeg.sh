#!/bin/bash
# 过滤 libjpeg 的 Corrupt JPEG 刷屏消息，保留其他 stderr 输出
exec "$@" 2> >(sed -u '/Corrupt JPEG/d' >&2)

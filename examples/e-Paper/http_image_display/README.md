# HTTP Image Display Example

这个示例演示如何从 HTTP 服务器下载二进制图像数据，并将其显示在 4.26 寸墨水屏上。

## 功能特性

- 通过 HTTP GET 请求从网络下载图像数据
- 支持 800x480 分辨率的墨水屏显示
- 自动等待网络连接
- 完整的错误处理和日志输出

## 硬件要求

- T5AI 开发板
- 4.26 寸墨水屏 (800x480 分辨率)
- Wi-Fi 网络连接

## 配置说明

在使用前，需要修改 `src/http_image_display.c` 中的以下配置：

```c
#define IMAGE_URL_HOST      "example.com"           // 修改为你的图片服务器地址
#define IMAGE_URL_PATH      "/image.bin"            // 修改为你的图片路径
```

## 图像格式要求

图像数据必须是原始二进制格式，符合墨水屏的要求：
- 分辨率: 800x480 像素
- 格式: 单色位图 (1 bit per pixel)
- 大小: 48000 字节 (800 * 480 / 8)
- 每个字节代表 8 个像素，MSB 在前

## 图像转换

你可以使用以下 Python 脚本将普通图片转换为墨水屏格式：

```python
from PIL import Image

# 打开图片并转换为黑白
img = Image.open('your_image.png')
img = img.resize((800, 480))
img = img.convert('1')  # 转换为单色

# 保存为原始二进制格式
with open('image.bin', 'wb') as f:
    pixels = img.load()
    for y in range(480):
        for x in range(0, 800, 8):
            byte = 0
            for bit in range(8):
                if pixels[x + bit, y]:
                    byte |= (0x80 >> bit)
            f.write(bytes([byte]))
```

## 构建和运行

1. 激活 TuyaOpen 环境：
```bash
source export.sh
```

2. 配置项目（如需要）：
```bash
tos.py config
```

3. 构建项目：
```bash
tos.py build
```

4. 烧录到设备：
```bash
tos.py flash
```

5. 查看日志输出：
```bash
tos.py monitor
```

## 工作流程

1. 程序启动后初始化网络管理器
2. 等待 Wi-Fi 连接建立
3. 网络连接成功后，自动发起 HTTP GET 请求下载图像
4. 下载完成后，初始化墨水屏
5. 将下载的图像数据显示在墨水屏上
6. 显示完成后，墨水屏进入休眠模式

## 日志输出示例

```
[DEBUG] Initializing network manager...
[DEBUG] HTTP Image Display example started
[DEBUG] Waiting for network connection...
[DEBUG] Network is up! Starting image download...
[DEBUG] Downloading image from http://example.com/image.bin
[DEBUG] HTTP response status: 200
[DEBUG] Response body length: 48000 bytes
[DEBUG] Successfully downloaded 48000 bytes
[DEBUG] Initializing e-Paper display...
[DEBUG] Displaying image on e-Paper...
[DEBUG] Image displayed successfully
[DEBUG] Putting e-Paper to sleep...
```

## 故障排除

### 网络连接失败
- 确保设备已正确配置 Wi-Fi 凭据
- 检查网络连接状态

### HTTP 下载失败
- 验证服务器地址和路径是否正确
- 确保服务器可访问
- 检查防火墙设置

### 图像显示异常
- 确认图像大小为 48000 字节
- 验证图像格式是否正确
- 检查墨水屏连接

## 扩展建议

- 添加 HTTPS 支持以提高安全性
- 实现图像缓存机制
- 支持多种图像格式
- 添加定时刷新功能
- 实现 OTA 更新图像内容

## 参考资料

- [TuyaOpen 文档](https://developer.tuya.com)
- [HTTP Client 示例](../protocols/http_client/)
- [墨水屏驱动示例](../e-Paper/4.26inch_e-Paper/)

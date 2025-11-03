#!/bin/bash
# =====================================================
#  ARMHF 平台静态编译 tcpdump 与 libpcap
#  作者: ChatGPT (GPT-5)
#  目标: arm-linux-gnueabihf 静态可执行文件
# =====================================================

set -e

# -------------------------------
# 1. 环境准备
# -------------------------------
echo ">>> 安装依赖包..."
sudo apt-get update -y
sudo apt-get install -y curl xz-utils build-essential automake autoconf libtool pkg-config flex

# -------------------------------
# 2. 下载并安装交叉编译工具链
# -------------------------------
TOOLCHAIN_URL="https://releases.linaro.org/components/toolchain/binaries/6.3-2017.05/arm-linux-gnueabihf/gcc-linaro-6.3.1-2017.05-x86_64_arm-linux-gnueabihf.tar.xz"
TOOLCHAIN_DIR="/opt/gcc-linaro-6.3.1-2017.05-x86_64_arm-linux-gnueabihf"
TOOLCHAIN_TAR=$(basename "$TOOLCHAIN_URL")

if [ ! -d "$TOOLCHAIN_DIR" ]; then
    echo ">>> 下载 Linaro 工具链..."
    curl -LO "$TOOLCHAIN_URL"

    echo ">>> 解压到 /opt ..."
    sudo tar -xf "$TOOLCHAIN_TAR" -C /opt/
else
    echo ">>> 工具链已存在，跳过下载。"
fi

export PATH=$TOOLCHAIN_DIR/bin:$PATH
export CROSS_COMPILE=arm-linux-gnueabihf-
export CC=${CROSS_COMPILE}gcc
export AR=${CROSS_COMPILE}ar
export LD=${CROSS_COMPILE}ld
export STRIP=${CROSS_COMPILE}strip


echo ">>> 使用交叉编译器: $(${CC} -v 2>&1 | tail -n 1)"

# -------------------------------
# 3. 下载源码
# -------------------------------
echo ">>> 下载 libpcap 与 tcpdump 源码..."
curl -LO https://www.tcpdump.org/release/libpcap-1.10.5.tar.gz
curl -LO https://www.tcpdump.org/release/tcpdump-4.99.0.tar.gz

tar xzf libpcap-1.10.5.tar.gz
tar xzf tcpdump-4.99.0.tar.gz

# -------------------------------
# 4. 编译 libpcap (静态)
# -------------------------------
echo ">>> 编译 libpcap (静态)..."
cd libpcap-1.10.5
make distclean >/dev/null 2>&1 || true

./configure \
    --host=arm-linux-gnueabihf \
    --enable-static \
    --disable-shared \
    CC=${CC} \
    LDFLAGS="-static"

make -j$(nproc)
cd ..

# -------------------------------
# 5. 编译 tcpdump (静态)
# -------------------------------
echo ">>> 编译 tcpdump (静态)..."
cd tcpdump-4.99.0
make distclean >/dev/null 2>&1 || true

./configure \
    --host=arm-linux-gnueabihf \
    --with-crypto=no \
    --disable-shared \
    --enable-static \
    CC=${CC} \
    LDFLAGS="-static" \
    CFLAGS="-I../libpcap-1.10.5 -include fcntl.h" \
    LIBS="../libpcap-1.10.5/libpcap.a"

make -j$(nproc)

# 精简二进制文件
${STRIP} tcpdump || true
cd ..

# -------------------------------
# 6. 输出结果
# -------------------------------
echo ">>> 静态编译完成！"
echo "生成文件路径: tcpdump-4.99.0/tcpdump"
file tcpdump-4.99.0/tcpdump
ls -lh tcpdump-4.99.0/tcpdump
echo
echo "✅ ARMhf 静态版 tcpdump 已构建完成，可直接复制到目标 ARM Linux 系统运行。"

#!/bin/bash
set -e

# ===========================
# 1. 安装依赖
# ===========================
echo ">>> 安装编译依赖..."
sudo apt-get update -y
sudo apt-get install -y curl xz-utils build-essential

# ===========================
# 2. 下载 Linaro 交叉编译工具链
# ===========================
TOOLCHAIN_URL="https://releases.linaro.org/components/toolchain/binaries/6.3-2017.05/arm-linux-gnueabihf/gcc-linaro-6.3.1-2017.05-x86_64_arm-linux-gnueabihf.tar.xz"
TOOLCHAIN_DIR="/opt/gcc-linaro-6.3.1-2017.05-x86_64_arm-linux-gnueabihf"

if [ ! -d "$TOOLCHAIN_DIR" ]; then
    echo ">>> 下载 Linaro 工具链..."
    curl -LO "$TOOLCHAIN_URL"

    echo ">>> 解压到 /opt/ ..."
    sudo tar -xf gcc-linaro-6.3.1-2017.05-x86_64_arm-linux-gnueabihf.tar.xz -C /opt/
else
    echo ">>> 工具链已存在，跳过下载"
fi

export PATH=$TOOLCHAIN_DIR/bin:$PATH
export CROSS_COMPILE=arm-linux-gnueabihf-
export CC=${CROSS_COMPILE}gcc

# ===========================
# 3. 下载源码
# ===========================
echo ">>> 下载 libpcap 与 tcpdump 源码..."
curl -LO https://www.tcpdump.org/release/libpcap-1.10.6.tar.gz
curl -LO https://www.tcpdump.org/release/tcpdump-4.99.0.tar.gz

tar xzf libpcap-1.10.6.tar.gz
tar xzf tcpdump-4.99.0.tar.gz

# ===========================
# 4. 编译 libpcap (静态)
# ===========================
echo ">>> 编译 libpcap..."
cd libpcap-1.10.6
make clean || true

./configure \
    --host=arm-linux-gnueabihf \
    --enable-static \
    --disable-shared \
    CC=${CC} \
    LDFLAGS="-static"

make -j$(nproc)
cd ..

# ===========================
# 5. 编译 tcpdump (静态)
# ===========================
echo ">>> 编译 tcpdump..."
cd tcpdump-4.99.0
make clean || true

./configure \
    --host=arm-linux-gnueabihf \
    --with-crypto=no \
    --with-smi=no \
    --disable-smb \
    --disable-shared \
    --enable-static \
    CC=${CC} \
    LDFLAGS="-static" \
    CFLAGS="-I../libpcap-1.10.6" \
    LIBS="../libpcap-1.10.6/libpcap.a"

make -j$(nproc)
cd ..

# ===========================
# 6. 结果展示
# ===========================
echo ">>> 编译完成!"
file tcpdump-4.99.0/tcpdump || true
ls -lh tcpdump-4.99.0/tcpdump

echo ">>> 静态编译的 tcpdump 已生成于: tcpdump-4.99.0/tcpdump"

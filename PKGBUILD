# Maintainer: noisyloop
pkgname=rainfall
pkgver=1.0.0
pkgrel=1
pkgdesc="Matrix rain meets trippy arcade - a tiny terminal game for Omarchy"
arch=('x86_64' 'aarch64')
url="https://github.com/noisyloop/rainfall"
license=('MIT')
depends=('ncurses' 'gcc-libs')
makedepends=('git')
source=("git+$url.git")
sha256sums=('SKIP')

build() {
    cd "$srcdir/$pkgname"
    make
}

package() {
    cd "$srcdir/$pkgname"
    make PREFIX=/usr DESTDIR="$pkgdir" install
    install -Dm644 LICENSE "$pkgdir/usr/share/licenses/$pkgname/LICENSE"
}

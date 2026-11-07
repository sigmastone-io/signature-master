// render-icon <app.svg> <out.ico>
//
// Rasterizes the SVG at every size Windows asks for and packs the PNGs into one .ico. Windows
// has read PNG-compressed icon entries since Vista, so no DIB conversion is needed.

#include <QBuffer>
#include <QCoreApplication>
#include <QDataStream>
#include <QFile>
#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QSvgRenderer>

#include <cstdio>

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);
    const QStringList arguments = QCoreApplication::arguments();
    if (arguments.size() != 3) {
        std::fputs("usage: render-icon <app.svg> <out.ico>\n", stderr);
        return 2;
    }

    QSvgRenderer renderer(arguments.at(1));
    if (!renderer.isValid()) {
        std::fputs("the SVG could not be read\n", stderr);
        return 1;
    }

    const int sizes[] = {16, 20, 24, 32, 40, 48, 64, 96, 128, 256};
    QList<QByteArray> pngs;
    for (const int size : sizes) {
        QImage image(size, size, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        renderer.render(&painter, QRectF(0, 0, size, size));
        painter.end();
        QByteArray png;
        QBuffer buffer(&png);
        buffer.open(QIODevice::WriteOnly);
        image.save(&buffer, "PNG");
        pngs.append(png);
    }

    QFile out(arguments.at(2));
    if (!out.open(QIODevice::WriteOnly)) {
        std::fputs("the .ico could not be written\n", stderr);
        return 1;
    }
    QDataStream stream(&out);
    stream.setByteOrder(QDataStream::LittleEndian);
    const quint16 count = static_cast<quint16>(pngs.size());
    stream << quint16(0) << quint16(1) << count; // reserved, type icon, count
    quint32 offset = 6 + 16 * count;
    for (int i = 0; i < pngs.size(); ++i) {
        const int size = sizes[i];
        stream << quint8(size == 256 ? 0 : size) << quint8(size == 256 ? 0 : size) // 0 means 256
               << quint8(0) << quint8(0)    // palette, reserved
               << quint16(1) << quint16(32) // planes, bpp
               << quint32(pngs.at(i).size()) << offset;
        offset += static_cast<quint32>(pngs.at(i).size());
    }
    for (const QByteArray& png : pngs) {
        stream.writeRawData(png.constData(), png.size());
    }
    return 0;
}

#include <QCoreApplication>
#include <QGuiApplication>
#include <QXmlStreamReader>
#include <QFile>
#include <QImage>
#include <QPainter>
#include <iostream>

#define VERSION                 1
#define SUB_VERSION             0

struct ErrorInfo
{
    unsigned int id;
    QString text;
};

struct TileSet
{
    QString name;
    unsigned int width,height;
    unsigned int tile_width,tile_height;
    QColor background;
};

struct ImageInfo
{
    QString source;
    QColor transparent_color;
    bool has_transparent_color;
};

struct TextInfo
{
    QString content;
    unsigned int size;
    QColor color;
};

void Help();
QImage HandleImage(TileSet &tile_set,QXmlStreamReader &reader);
QImage HandleText(TileSet &tile_set,QXmlStreamReader &reader);
void HandleXMLFile(const QString &file_name);

int main(int argc, char *argv[])
{
    QGuiApplication a(argc, argv);

    if(argc!=2)
    {
        Help();
        return 1;
    }

    try {
        HandleXMLFile(argv[1]);
        return 0;
    } catch (ErrorInfo &error) {
        if(error.id==0)
        {
            Help();
            return 1;
        }
        else
        {
            std::cout<<error.id<<":"<<error.text.toStdString()<<std::endl;
            return 1;
        }
    }

    return a.exec();
}

QColor ValueToColor(const QString &value)
{
    if(value.length()<9)
        return QColor(0,0,0,0);

    auto red=value.mid(1,2).toUInt(nullptr,16);
    auto green=value.mid(3,2).toUInt(nullptr,16);
    auto blue=value.mid(5,2).toUInt(nullptr,16);
    auto alpha=value.mid(7,2).toUInt(nullptr,16);

    return QColor(red,green,blue,alpha);
}

void HandleTileSetAttributes(TileSet &tile_set,QXmlStreamReader &reader)
{
    auto attributes=reader.attributes();

    if(attributes.hasAttribute("name"))
        tile_set.name=attributes.value("name").toString();
    else
        tile_set.name="unamed";

    if(attributes.hasAttribute("width"))
        tile_set.width=attributes.value("width").toUInt();
    else
        tile_set.width=1;

    if(attributes.hasAttribute("height"))
        tile_set.height=attributes.value("height").toUInt();
    else
        tile_set.height=1;

    if(attributes.hasAttribute("tilewidth"))
        tile_set.tile_width=attributes.value("tilewidth").toUInt();
    else
        tile_set.tile_width=32;

    if(attributes.hasAttribute("tileheight"))
        tile_set.tile_height=attributes.value("tileheight").toUInt();
    else
        tile_set.tile_height=32;

    if(attributes.hasAttribute("background"))
    {
        tile_set.background=ValueToColor(attributes.value("background").toString());
    }
    else
        tile_set.background=QColor(0,0,0,0);
}

QImage MakeCanvas(TileSet &tile_set)
{
    QImage image(tile_set.width*tile_set.tile_width,tile_set.height*tile_set.tile_height,
                 QImage::Format_RGBA8888);

    image.fill(tile_set.background);
    return image;
}

bool HandleTileSet(QXmlStreamReader &reader)
{
    TileSet tile_set;

    HandleTileSetAttributes(tile_set,reader);

    QImage canvas=MakeCanvas(tile_set);

    QPainter painter(&canvas);

    unsigned int index=0;

    while(!reader.isEndElement())
    {
        auto token_type=reader.readNext();

        if(token_type==QXmlStreamReader::StartElement)
        {
            QImage tile_image;
            if(reader.name()=="image")
                tile_image=HandleImage(tile_set,reader);
            else if(reader.name()=="text")
                tile_image=HandleText(tile_set,reader);


            unsigned int x=(index%tile_set.width)*tile_set.tile_width;
            unsigned int y=(index/tile_set.width)*tile_set.tile_height;

            painter.drawImage(x,y,tile_image,0,0,tile_image.width(),tile_image.height(),Qt::ColorOnly);

            reader.readNext();
            index++;
        }
    }

    canvas.save(tile_set.name+".png");

    return true;
}

void HandleImageAttributes(ImageInfo &image_info,QXmlStreamReader &reader)
{
    auto attributes=reader.attributes();

    if(attributes.hasAttribute("source"))
        image_info.source=attributes.value("source").toString();
    else
    {
        ErrorInfo error;

        error.id=1;
        error.text="Failed to get image source!";

        throw error;
    }

    if(attributes.hasAttribute("transparentcolor"))
    {
        image_info.has_transparent_color=true;
        image_info.transparent_color=ValueToColor(attributes.value("transparentcolor").toString());
    }
    else
    {
        image_info.has_transparent_color=false;
    }
}

QImage HandleImage(TileSet &tile_set,QXmlStreamReader &reader)
{
    ImageInfo image_info;
    QImage tile_image(tile_set.tile_width,tile_set.tile_height,QImage::Format_RGBA8888);
    QPainter painter(&tile_image);

    HandleImageAttributes(image_info,reader);

    QImage source_image(image_info.source);

    QRect target_rect(0,0,tile_set.tile_width,tile_set.tile_height);

    QRect source_rect(0,0,source_image.width(),source_image.height());

    painter.drawImage(target_rect,source_image,source_rect);

    if(image_info.has_transparent_color)
    {
        QColor color(0,0,0,0);
        for(auto y=0;y<tile_image.height();y++)
            for(auto x=0;x<tile_image.width();x++)
                if(tile_image.pixelColor(x,y)==image_info.transparent_color)
                    tile_image.setPixelColor(x,y,color);
    }

    while(!reader.isEndElement())
    {
        reader.readNext();
    }

    return tile_image;
}

void HandleTextAttrbutes(TextInfo &text_info,QXmlStreamReader &reader)
{
    auto attributes=reader.attributes();

    if(attributes.hasAttribute("content"))
        text_info.content=attributes.value("content").toString();
    else
        text_info.content="T";

    if(attributes.hasAttribute("size"))
        text_info.size=attributes.value("size").toUInt();
    else
        text_info.size=16;

    if(attributes.hasAttribute("color"))
        text_info.color=ValueToColor(attributes.value("color").toString());
    else
        text_info.color=QColor(255,255,0,255);
}

QImage HandleText(TileSet &tile_set,QXmlStreamReader &reader)
{
    QImage tile_image(tile_set.tile_width,tile_set.tile_height,QImage::Format_RGBA8888);
    QColor color(0,0,0,0);

    tile_image.fill(color);
    QPainter painter(&tile_image);

    TextInfo text_info;

    HandleTextAttrbutes(text_info,reader);

    while(!reader.isEndElement())
    {
        reader.readNext();
    }

    QFont font("Times", text_info.size, QFont::Bold);

    painter.setFont(font);

     QRect rect(0,0,tile_set.tile_width,tile_set.tile_height);

     painter.setPen(text_info.color);
     painter.drawText(rect,Qt::AlignCenter,text_info.content);

    return tile_image;
}

void HandleXMLFile(const QString &file_name)
{
    QFile xml_file(file_name);

    if(!xml_file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        ErrorInfo error;

        error.id=0;

        error.text="Failed to open file!";

        throw error;
    }

    QXmlStreamReader reader(&xml_file);

    while(!reader.atEnd())
    {
        auto token_type=reader.readNext();

        if(token_type==QXmlStreamReader::StartElement ||
                reader.name()=="tileset")
        {
            HandleTileSet(reader);
        }
    }

    if(reader.hasError())
    {
        ErrorInfo error;

        throw error;
    }
}

void Help()
{
    std::cout<<"TileMerge"<<" v"<<VERSION<<"."<<SUB_VERSION<<std::endl;
}

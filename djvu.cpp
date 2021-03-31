#include "djvu.h"

#include <QtPlugin>
#include <QFileInfo>
#include <QFile>
#include <QByteArray>

#include <QtDebug>

/**
 * Constructor
 */
DJVU::DJVU() :
        currPage ( 0 ),
        image ( NULL ),
        imageBuf ( NULL ),
        zoom ( 40 )
{
}


/**
 * Destructor
 */
DJVU::~DJVU()
{
    clean();
}


QImage * DJVU::getImage()
{
    return image;
}


bool DJVU::nextPage()
{
    if ( image && currPage < ddjvu_document_get_pagenum ( document ) - 1 )
    {
        currPage++;
        renderPage();
        return true;
    }
    return false;
}


bool DJVU::prevPage()
{
    if ( image && currPage > 0 )
    {
        currPage--;
        renderPage();
        return true;
    }
    return false;
}


void DJVU::firstPage()
{
    if ( image && currPage )
    {
        currPage = 0;
        renderPage();
    }
}


void DJVU::lastPage()
{
    if ( image && currPage != ddjvu_document_get_pagenum ( document ) - 1 )
    {
        currPage = ddjvu_document_get_pagenum ( document ) - 1;
        renderPage();
    }
}

/**
 * Gets number of current page
 * @return
 */
int DJVU::getNumPage()
{
    return currPage;
}

void DJVU::setZoom ( int _zoom )
{
    if ( image )
    {
        zoom = _zoom;
        renderPage();
    }
}


int DJVU::getZoom()
{ return zoom; }


bool DJVU::openDocument ( const QString & fileName )
{
    QFileInfo info ( fileName );
    if ( !info.isReadable() )
    {
        qWarning() << "cannot read file";
        return false;
    }

    context = ddjvu_context_create ( "eyepiece" );
    if ( !context )
    {
        qWarning() << "cannot create context";
        clean();
        return false;
    }

    QByteArray b = QFile::encodeName ( fileName );
    if ( ! ( document = ddjvu_document_create_by_filename ( context, b, 0 ) ) )
    {
        qWarning() << "cannot create decoder";
        clean();
        return false;
    }

    handle_ddjvu_messages ( context, true /*false*/ ); // TODO: file is valid?

    renderPage();
    return true;
}


void DJVU::renderPage()
{
    page = ddjvu_page_create_by_pageno ( document, currPage ); // pages, from 0
    if ( !page )
    {
        clean();
        return;
    }

    int resolution = ddjvu_page_get_resolution ( page );
    qWarning() << "Page resolution: " << resolution;

    while ( !ddjvu_page_decoding_done ( page ) ) {1;/* we just kill time (FIXME : inefficient) */}

    pageRect.w = ddjvu_page_get_width ( page );
    pageRect.h = ddjvu_page_get_height ( page );

    if ( pageRect.w < 1 || pageRect.h < 1 )
    {
        clean();
        return;
    }

    //ddjvu_page_set_rotation ( page, DDJVU_ROTATE_0 ); // need?
    pageRect.x = 0;
    pageRect.y = 0;
    pageRect.w = ( int ) ( pageRect.w * zoom * 72.0 / 100.0 / resolution );
    pageRect.h = ( int ) ( pageRect.h * zoom * 72.0 / 100.0 / resolution );
    rendRect = pageRect;

    row_stride = pageRect.w * 4; // !!!!!!!!!!

    qWarning() << "W: " << pageRect.w << "  H:" << pageRect.h;

    static uint masks[4] = { 0xff0000, 0xff00, 0xff, 0xff000000 };
    format = ddjvu_format_create ( DDJVU_FORMAT_RGBMASK32, 4, masks );
    if ( !format )
    {
        clean();
        return;
    }

    ddjvu_format_set_row_order ( format, 1 );

    if ( imageBuf ) delete imageBuf;
    imageBuf = new ( uchar[ row_stride * pageRect.h ] );

    if ( image ) delete image;
    image = new QImage ( imageBuf, pageRect.w, pageRect.h, row_stride, QImage::Format_RGB32 );

    if ( !ddjvu_page_render ( page, DDJVU_RENDER_COLOR,
                              &pageRect,
                              &rendRect,
                              format,
                              row_stride,
                              ( char* ) imageBuf ) )
    {
        delete image;
        delete imageBuf;
    }
}


void DJVU::handle_ddjvu_messages ( ddjvu_context_t * ctx, int wait )
{
    const ddjvu_message_t *msg;
    if ( wait )
        ddjvu_message_wait ( ctx );
    while ( ( msg = ddjvu_message_peek ( ctx ) ) )
    {
        switch ( msg->m_any.tag )
        {
            case DDJVU_ERROR:      /*....*/ ; break;
            case DDJVU_INFO:       /*....*/ ; break;
            case DDJVU_NEWSTREAM:  /*....*/ ; break;
                //   ....
            default: break;
        }
        ddjvu_message_pop ( ctx );
    }
}


void DJVU::clean()
{
    if ( page )     ddjvu_page_release ( page );
    if ( document ) ddjvu_document_release ( document );
    if ( context )  ddjvu_context_release ( context );
    if ( format )   ddjvu_format_release ( format );
    if ( image )    delete image;
    if ( imageBuf ) delete imageBuf;
}

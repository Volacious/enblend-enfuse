/************************************************************************/
/*                                                                      */
/*               Copyright 2001-2002 by Gunnar Kedenburg                */
/*       Cognitive Systems Group, University of Hamburg, Germany        */
/*                                                                      */
/*    This file is part of the VIGRA computer vision library.           */
/*    ( Version 1.2.0, Aug 07 2003 )                                    */
/*    You may use, modify, and distribute this software according       */
/*    to the terms stated in the LICENSE file included in               */
/*    the VIGRA distribution.                                           */
/*                                                                      */
/*    The VIGRA Website is                                              */
/*        http://kogs-www.informatik.uni-hamburg.de/~koethe/vigra/      */
/*    Please direct questions, bug reports, and contributions to        */
/*        koethe@informatik.uni-hamburg.de                              */
/*                                                                      */
/*  THIS SOFTWARE IS PROVIDED AS IS AND WITHOUT ANY EXPRESS OR          */
/*  IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED      */
/*  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. */
/*                                                                      */
/************************************************************************/

#ifdef HasJPEG

#include <stdexcept>
#include <csetjmp>
#include "void_vector.hxx"
#include "error.hxx"
#include "auto_file.hxx"
#include "jpeg.hxx"

extern "C"
{
#include <jpeglib.h>
}

namespace vigra
{
    CodecDesc JPEGCodecFactory::getCodecDesc() const
    {
        CodecDesc desc;

        // init file type
        desc.fileType = "JPEG";

        // init pixel types
        desc.pixelTypes.resize(1);
        desc.pixelTypes[0] = "UINT8";

        // init compression types
        desc.compressionTypes.resize(1);
        desc.compressionTypes[0] = "JPEG";

        // init magic strings
        desc.magicStrings.resize(1);
        desc.magicStrings[0].resize(3);
        desc.magicStrings[0][0] = '\377';
        desc.magicStrings[0][1] = '\330';
        desc.magicStrings[0][2] = '\377';

        // init file extensions
        desc.fileExtensions.resize(2);
        desc.fileExtensions[0] = "jpg";
        desc.fileExtensions[1] = "jpeg";

        return desc;
    }

    std::auto_ptr<Decoder> JPEGCodecFactory::getDecoder() const
    {
        return std::auto_ptr<Decoder>( new JPEGDecoder() );
    }

    std::auto_ptr<Encoder> JPEGCodecFactory::getEncoder() const
    {
        return std::auto_ptr<Encoder>( new JPEGEncoder() );
    }

    class JPEGCodecImpl
    {
        // extend the jpeg_error_mgr by a jump buffer
        struct error_mgr
        {
            jpeg_error_mgr pub;
            jmp_buf buf;
        };

    public:

        // attributes

        error_mgr err;

        // methods

        static void longjumper( j_common_ptr info );
    };

    void JPEGCodecImpl::longjumper( j_common_ptr info )
    {
        (*info->err->output_message)(info);
        error_mgr * error = reinterpret_cast< error_mgr * >(info->err);
        longjmp( error->buf, 1 );
    }

    class JPEGDecoderImplBase : public JPEGCodecImpl
    {
    public:

        // attributes
        jpeg_decompress_struct info;

        JPEGDecoderImplBase()
        {
            // create the decompression struct
            jpeg_create_decompress(&info);
        }

        virtual ~JPEGDecoderImplBase()
        {
            // delete the decompression struct
            jpeg_destroy_decompress(&info);
        }
    };

    class JPEGEncoderImplBase : public JPEGCodecImpl
    {
    public:

        // attributes
        jpeg_compress_struct info;

        JPEGEncoderImplBase()
        {
            // create the decompression struct
            jpeg_create_compress(&info);
        }

        virtual ~JPEGEncoderImplBase()
        {
            // delete the decompression struct
            jpeg_destroy_compress(&info);
        }
    };

    struct JPEGDecoderImpl : public JPEGDecoderImplBase
    {
        // attributes

        auto_file file;
        void_vector<JSAMPLE> bands;
        unsigned int width, height, components, scanline;

        // ctor, dtor

        JPEGDecoderImpl( const std::string & filename );
        ~JPEGDecoderImpl();

        // methods

        void init();
    };

    JPEGDecoderImpl::JPEGDecoderImpl( const std::string & filename )
#ifdef WIN32
        : file( filename.c_str(), "rb" ),
#else
        : file( filename.c_str(), "r" ),
#endif
          bands(0), scanline(0)
    {
        // setup setjmp() error handling
        info.err = jpeg_std_error( ( jpeg_error_mgr * ) &err );
        err.pub.error_exit = &longjumper;

        // setup the data source
        if (setjmp(err.buf)) {
            vigra_fail( "error in jpeg_stdio_src()" );
        }
        jpeg_stdio_src( &info, file.get() );
    }

    void JPEGDecoderImpl::init()
    {
        // read the header
        if (setjmp(err.buf))
            vigra_fail( "error in jpeg_read_header()" );
        jpeg_read_header( &info, TRUE );

        // start the decompression
        if (setjmp(err.buf))
            vigra_fail( "error in jpeg_start_decompress()" );
        jpeg_start_decompress(&info);

        // transfer interesting header information
        width = info.output_width;
        height = info.output_height;
        components = info.output_components;

        // alloc memory for a single scanline
        bands.resize( width * components );

        // set colorspace
        info.jpeg_color_space = components == 1 ? JCS_GRAYSCALE : JCS_RGB;
    }

    JPEGDecoderImpl::~JPEGDecoderImpl()
    {
    }

    void JPEGDecoder::init( const std::string & filename )
    {
        pimpl = new JPEGDecoderImpl(filename);
        pimpl->init();
    }

    JPEGDecoder::~JPEGDecoder()
    {
        delete pimpl;
    }

    std::string JPEGDecoder::getFileType() const
    {
        return "JPEG";
    }

    unsigned int JPEGDecoder::getWidth() const
    {
        return pimpl->width;
    }

    unsigned int JPEGDecoder::getHeight() const
    {
        return pimpl->height;
    }

    unsigned int JPEGDecoder::getNumBands() const
    {
        return pimpl->components;
    }

    std::string JPEGDecoder::getPixelType() const
    {
        return "UINT8";
    }

    unsigned int JPEGDecoder::getOffset() const
    {
        return pimpl->components;
    }

    const void * JPEGDecoder::currentScanlineOfBand( unsigned int band ) const
    {
        return pimpl->bands.data() + band;
    }

    void JPEGDecoder::nextScanline()
    {
        // check if there are scanlines left at all, eventually read one
        JSAMPLE * band = pimpl->bands.data();
        if ( pimpl->info.output_scanline < pimpl->info.output_height ) {
            if (setjmp(pimpl->err.buf))
                vigra_fail( "error in jpeg_read_scanlines()" );
            jpeg_read_scanlines( &pimpl->info, &band, 1 );
        }
    }

    void JPEGDecoder::close()
    {
        // finish any pending decompression
        if (setjmp(pimpl->err.buf))
            vigra_fail( "error in jpeg_finish_decompress()" );
        jpeg_finish_decompress(&pimpl->info);
    }

    void JPEGDecoder::abort() {}

    struct JPEGEncoderImpl : public JPEGEncoderImplBase
    {
        // attributes

        auto_file file;
        void_vector<JSAMPLE> bands;
        unsigned int width, height, components, scanline;
        int quality;

        // state
        bool finalized;

        // ctor, dtor

        JPEGEncoderImpl( const std::string & filename );
        ~JPEGEncoderImpl();

        // methods

        void finalize();
    };

    JPEGEncoderImpl::JPEGEncoderImpl( const std::string & filename )
#ifdef WIN32
        : file( filename.c_str(), "wb" ),
#else
        : file( filename.c_str(), "w" ),
#endif
          scanline(0), quality(-1),
          finalized(false)
    {
        // setup setjmp() error handling
        info.err = jpeg_std_error( ( jpeg_error_mgr * ) &err );
        err.pub.error_exit = &longjumper;

        // setup the data dest
        if (setjmp(err.buf)) {
            vigra_fail( "error in jpeg_stdio_dest()" );
        }
        jpeg_stdio_dest( &info, file.get() );
    }

    JPEGEncoderImpl::~JPEGEncoderImpl()
    {
    }

    void JPEGEncoderImpl::finalize()
    {
        VIGRA_IMPEX2_FINALIZED(finalized);

        // alloc memory for a single scanline
        bands.resize( width * components );
        finalized = true;

        // init the compression
        info.image_width = width;
        info.image_height = height;
        info.input_components = components;

        // rgb or gray can be assumed here
        info.in_color_space = components == 1 ? JCS_GRAYSCALE : JCS_RGB;
        info.X_density = 100;
        info.Y_density = 100;

        // set defaults based upon the set values
        if (setjmp(err.buf))
            vigra_fail( "error in jpeg_set_defaults()" );
        jpeg_set_defaults(&info);

        // set the quality level
        if ( quality != -1 ) {
            if (setjmp(err.buf))
                vigra_fail( "error in jpeg_set_quality()" );
            jpeg_set_quality( &info, quality, TRUE );
        }

        // enhance the quality a little bit
        for ( unsigned int i = 0; i < MAX_COMPONENTS; ++i ) {
            info.comp_info[i].h_samp_factor = 1;
            info.comp_info[i].v_samp_factor = 1;
        }
#ifdef C_ARITH_CODING_SUPPORTED
        info.arith_code = TRUE;
#endif
#ifdef ENTROPY_OPT_SUPPORTED
        info.optimize_coding = TRUE;
#endif

        // start the compression
        if (setjmp(err.buf))
            vigra_fail( "error in jpeg_start_compress()" );
        jpeg_start_compress( &info, TRUE );
    }

    void JPEGEncoder::init( const std::string & filename )
    {
        pimpl = new JPEGEncoderImpl(filename);
    }

    JPEGEncoder::~JPEGEncoder()
    {
        delete pimpl;
    }

    std::string JPEGEncoder::getFileType() const
    {
        return "JPEG";
    }

    void JPEGEncoder::setWidth( unsigned int width )
    {
        VIGRA_IMPEX2_FINALIZED(pimpl->finalized);
        pimpl->width = width;
    }

    void JPEGEncoder::setHeight( unsigned int height )
    {
        VIGRA_IMPEX2_FINALIZED(pimpl->finalized);
        pimpl->height = height;
    }

    void JPEGEncoder::setNumBands( unsigned int bands )
    {
        VIGRA_IMPEX2_FINALIZED(pimpl->finalized);
        pimpl->components = bands;
    }

    void JPEGEncoder::setCompressionType( const std::string & comp,
                                          int quality )
    {
        VIGRA_IMPEX2_FINALIZED(pimpl->finalized);
        if ( comp == "LOSSLESS" )
            vigra_fail( "lossless encoding is not supported by"
                        " the jpeg implementation impex uses." );
        pimpl->quality = quality;
    }

    void JPEGEncoder::setPixelType( const std::string & pixelType )
    {
        VIGRA_IMPEX2_FINALIZED(pimpl->finalized);
        if ( pixelType != "UINT8" )
            vigra_precondition( false, "only UINT8 pixels are supported." );
    }

    unsigned int JPEGEncoder::getOffset() const
    {
        return pimpl->components;
    }

    void JPEGEncoder::finalizeSettings()
    {
        pimpl->finalize();
    }

    void * JPEGEncoder::currentScanlineOfBand( unsigned int band )
    {
        return pimpl->bands.data() + band;
    }

    void JPEGEncoder::nextScanline()
    {
        // check if there are scanlines left at all, eventually write one
        JSAMPLE * band = pimpl->bands.data();
        if ( pimpl->info.next_scanline < pimpl->info.image_height ) {
            if (setjmp(pimpl->err.buf))
                vigra_fail( "error in jpeg_write_scanlines()" );
            jpeg_write_scanlines( &pimpl->info, &band, 1 );
        }
    }

    void JPEGEncoder::close()
    {
        // finish any pending compression
        if (setjmp(pimpl->err.buf))
            vigra_fail( "error in jpeg_finish_compress()" );
        jpeg_finish_compress( &pimpl->info );
    }

    void JPEGEncoder::abort() {}
}

#endif // HasJPEG

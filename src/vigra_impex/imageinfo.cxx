/************************************************************************/
/*                                                                      */
/*               Copyright 2002 by Gunnar Kedenburg                     */
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
/* Modifications by Pablo d'Angelo, as of 3 July 2004:
 *  - Added UINT16 and UINT32 pixel types.
 *  - Added support for obtaining extra bands beyond RGB.
 *  - Added support for a position field that indicates the start of this
 *    image relative to some global origin.
 *  - Added support for x and y resolution fields.
 */

#include <iostream>
#include <algorithm>
#include <iterator>
#include <string>
#include <sstream>
#include <vector>
#include <iterator>
#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vigra/imageinfo.hxx"
#include "codecmanager.hxx"

#if defined(_WIN32)
#  include <windows.h>
#else
#  include <dirent.h>
#endif

namespace vigra
{

namespace detail
{

struct NumberCompare
{
    bool operator()(std::string const & l, std::string const & r) const
    {
        return atoi(l.c_str()) < atoi(r.c_str());
    }
};

} // namespace detail


// find filenames matching the pattern "<path>/base[0-9]+ext"
#ifdef _WIN32
void findImageSequence(const std::string &name_base,
                       const std::string &name_ext,
                       std::vector<std::string> & numbers)
{
    // find out how many images we have
    BOOL            fFinished;
    HANDLE          hList;
    TCHAR           szDir[MAX_PATH+1];
    WIN32_FIND_DATA FileData;

    std::string base, path;
    size_t split = name_base.rfind('/');
    if(split == -1)
    {
        path = ".";
        base = name_base;
    }
    else
    {
        for(size_t i=0; i<split; ++i)
        {
            if(name_base[i] == '/')
                path += '\\';
            else
                path += name_base[i];
        }
        base.append(name_base, split+1, name_base.size() - split - 1);
    }

    std::vector<std::string> result;
    char numbuf[21], extbuf[1024];
    std::string pattern = base + "%20[0-9]%1023s";

    // Get the proper directory path
    sprintf(szDir, "%s\\%s*%s", path.c_str(), base.c_str(), name_ext.c_str());

    // Get the first file
    hList = FindFirstFile(szDir, &FileData);
    if (hList == INVALID_HANDLE_VALUE)
    {
        std::string message("importVolume(): No files matching '");
        message = message + szDir + "'.";
        vigra_fail(message.c_str());
    }
    else
    {
        // Traverse through the directory structure
        fFinished = FALSE;
        while (!fFinished)
        {
            if(sscanf(FileData.cFileName, pattern.c_str(), numbuf, extbuf) == 2)
            {
                if(strcmp(name_ext.c_str(), extbuf) == 0)
                    result.push_back(std::string(numbuf));
            }
            if (!FindNextFile(hList, &FileData))
            {
                if (GetLastError() == ERROR_NO_MORE_FILES)
                {
                    fFinished = TRUE;
                }
            }
        }
    }

    FindClose(hList);

    std::sort(result.begin(), result.end(), detail::NumberCompare());
    numbers.swap(result);
}

#else // _WIN32

void findImageSequence(const std::string &name_base,
                       const std::string &name_ext,
                       std::vector<std::string> & numbers)
{
    // find out how many images we have
    std::string base, path;
    int split = name_base.rfind('/');
    if(split == -1)
    {
        path = ".";
        base = name_base;
    }
    else
    {
        path.append(name_base, 0, split);
        base.append(name_base, split+1, name_base.size() - split - 1);
    }

    DIR * dir = opendir(path.c_str());
    if(!dir)
    {
        std::string message("importVolume(): Unable to open directory '");
        message = message + path + "'.";
        vigra_fail(message.c_str());
    }

    std::vector<std::string> result;
    dirent * dp;
    errno = 0;
    char numbuf[21], extbuf[1024];
    std::string pattern = base + "%20[0-9]%1023s";
    while ((dp = readdir(dir)) != NULL)
    {
        if(sscanf(dp->d_name, pattern.c_str(), numbuf, extbuf) == 2)
        {
            if(strcmp(name_ext.c_str(), extbuf) == 0)
                result.push_back(std::string(numbuf));
        }
    }

    closedir(dir);

    vigra_precondition(errno == 0,
          "importVolume(): I/O error while searching for images.");

    std::sort(result.begin(), result.end(), detail::NumberCompare());
    numbers.swap(result);
}

#endif // _WIN32

// build a string from a sequence.
#if _MSC_VER < 1300
template <class iterator>
std::string stringify (const iterator &start, const iterator &end)
{
    return stringifyImpl(start, end, *start);
}

template <class iterator, class Value>
std::string stringifyImpl (const iterator &start, const iterator &end, Value const &)
{
    std::ostringstream out;
    // do not place a space character after the last sequence element.
    std::copy (start, end - 1,
               std::ostream_iterator <Value> (out, " "));
    out << *(end-1);
    return out.str ();
}

#else

template <class iterator>
std::string stringify (const iterator &start, const iterator &end)
{
    typedef typename std::iterator_traits<iterator>::value_type value_type;
    std::ostringstream out;
    // do not place a space character after the last sequence element.
    std::copy (start, end - 1,
               std::ostream_iterator <value_type> (out, " "));
    out << *(end-1);
    return out.str ();
}

#endif // _MSC_VER < 1300

void validate_filetype( std::string filetype )
{
    vigra_precondition( codecManager().fileTypeSupported(filetype),
                        "given file type is not supported" );
}

std::string impexListFormats()
{
    std::vector<std::string> ft = codecManager().supportedFileTypes();
    return stringify( ft.begin(), ft.end() );
}

std::string impexListExtensions()
{
    std::vector<std::string> ft = codecManager().supportedFileExtensions();
    return stringify( ft.begin(), ft.end() );
}

bool isImage(char const * filename)
{
    return CodecManager::manager().getFileTypeByMagicString(filename) != "";
}

// class ImageExportInfo

ImageExportInfo::ImageExportInfo( const char * filename )
    : m_filename(filename),
      m_x_res(0), m_y_res(0)
{}

ImageExportInfo & ImageExportInfo::setFileType( const char * filetype )
{
    m_filetype = filetype;
    return *this;
}

ImageExportInfo & ImageExportInfo::setCompression( const char * comp )
{
    m_comp = comp;
    return *this;
}

const char * ImageExportInfo::getFileName() const
{
    return m_filename.c_str();
}

const char * ImageExportInfo::getFileType() const
{
    return m_filetype.c_str();
}

ImageExportInfo & ImageExportInfo::setPixelType( const char * s )
{
    m_pixeltype = s;
    return *this;
}

const char * ImageExportInfo::getPixelType() const
{
    return m_pixeltype.c_str();
}

const char * ImageExportInfo::getCompression() const
{
    return m_comp.c_str();
}

float ImageExportInfo::getXResolution() const
{
    return m_x_res;
}

float ImageExportInfo::getYResolution() const
{
    return m_y_res;
}

ImageExportInfo & ImageExportInfo::setXResolution( float val )
{
    m_x_res = val;
    return *this;
}

ImageExportInfo & ImageExportInfo::setYResolution( float val )
{
    m_y_res = val;
    return *this;
}

ImageExportInfo & ImageExportInfo::setPosition(const vigra::Diff2D & pos)
{
    m_pos = pos;
    return *this;
}

vigra::Diff2D ImageExportInfo::getPosition() const
{
    return m_pos;
}

// return an encoder for a given ImageExportInfo object
std::auto_ptr<Encoder> encoder( const ImageExportInfo & info )
{
    std::auto_ptr<Encoder> enc;

    std::string filetype = info.getFileType();
    if ( filetype != "" ) {
        validate_filetype(filetype);
        std::auto_ptr<Encoder> enc2
            = getEncoder( std::string( info.getFileName() ), filetype );
        enc = enc2;
    } else {
        std::auto_ptr<Encoder> enc2
            = getEncoder( std::string( info.getFileName() ) );
        enc = enc2;
    }

    std::string comp = info.getCompression();
    if ( comp != "" ) {

        // check for JPEG compression
        int quality = -1;
        std::istringstream compstream(comp.c_str());
        compstream >> quality;
        if ( quality != -1 ) {
            enc->setCompressionType( "JPEG", quality );
            return enc;
        }

        // leave any other compression type to the codec
        enc->setCompressionType(comp);
    }

    std::string pixel_type = info.getPixelType();
    if ( pixel_type != "" ) {
        enc->setPixelType(pixel_type);
    }

    // set other properties
    enc->setXResolution(info.getXResolution());
    enc->setYResolution(info.getYResolution());
    enc->setPosition(info.getPosition());

    return enc;
}

// class ImageImportInfo

ImageImportInfo::ImageImportInfo( const char * filename )
    : m_filename(filename)
{
    std::auto_ptr<Decoder> decoder = getDecoder(m_filename);

    m_filetype = decoder->getFileType();
    m_pixeltype = decoder->getPixelType();
    m_width = decoder->getWidth();
    m_height = decoder->getHeight();
    m_num_bands = decoder->getNumBands();
    m_num_extra_bands = decoder->getNumExtraBands();
    m_pos = decoder->getPosition();

    decoder->abort(); // there probably is no better way than this
}

const char * ImageImportInfo::getFileName() const
{
    return m_filename.c_str();
}

const char * ImageImportInfo::getFileType() const
{
    return m_filetype.c_str();
}

const char * ImageImportInfo::getPixelType() const
{
    return m_pixeltype.c_str();
}

ImageImportInfo::PixelType ImageImportInfo::pixelType() const
{
   const std::string pixeltype=getPixelType();
   if (pixeltype == "UINT8")
     return UINT8;
   if (pixeltype == "INT16")
     return INT16;
   if (pixeltype == "INT32")
     return INT32;
   if (pixeltype == "FLOAT")
     return FLOAT;
   if (pixeltype == "DOUBLE")
     return DOUBLE;
   vigra_fail( "internal error: unknown pixel type" );
   return ImageImportInfo::PixelType();
}

int ImageImportInfo::width() const
{
    return m_width;
}

int ImageImportInfo::height() const
{
    return m_height;
}

int ImageImportInfo::numBands() const
{
    return m_num_bands;
}

int ImageImportInfo::numExtraBands() const
{
    return m_num_extra_bands;
}

vigra::Size2D ImageImportInfo::size() const
{
    return vigra::Size2D( m_width, m_height );
}

bool ImageImportInfo::isGrayscale() const
{
    return m_num_bands == 1;
}

bool ImageImportInfo::isColor() const
{
    return (m_num_bands - m_num_extra_bands) == 3;
}

bool ImageImportInfo::isByte() const
{
    return m_pixeltype == "UINT8";
}

vigra::Diff2D ImageImportInfo::getPosition() const
{
    return m_pos;
}

float ImageImportInfo::getXResolution() const
{
    return m_x_res;
}

float ImageImportInfo::getYResolution() const
{
    return m_y_res;
}

// return a decoder for a given ImageImportInfo object
std::auto_ptr<Decoder> decoder( const ImageImportInfo & info )
{
    std::string filetype = info.getFileType();
    validate_filetype(filetype);
    return getDecoder( std::string( info.getFileName() ), filetype );
}

} // namespace vigra

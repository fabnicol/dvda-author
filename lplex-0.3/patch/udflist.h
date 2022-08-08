// Interface routines for dvdread-0.9.7_udflist.patch.
// Lists the contents of a udf image.

#ifndef UDFLIST_H
#define UDFLIST_H

#include <dvdread/dvd_reader.h>

#ifdef __cplusplus
extern "C" {
#endif

// c interface:
   
// ----------------------------------------------------------------------------
//    udflist_cb_t : 
// ----------------------------------------------------------------------------
//    Callback prototype.
//
//    Arguments:
//       <fname>        - full path to item
//       <ftype>        - 4=directory 5=file
//       <lb>           - logical block number (2048 bytes each)
//       <len>          - filesize in bytes
//       <device>       - dvd_reader_t instance
//
//    Return anything (ignored).
// ----------------------------------------------------------------------------
   
typedef int (*udflist_cb_t)( const char *fname, uint16_t ftype, uint32_t lb,
   uint32_t len, dvd_reader_t *device );

// ----------------------------------------------------------------------------
//    udflist : 
// ----------------------------------------------------------------------------
//    Generates a callback for each item within the given scope.
//
//    Arguments:
//       <device>       - dvd_reader_t instance
//       <filename>     - path to search under; root="", no wildcards.
//       <recursive>    - whether to drop down into subdirectories
//       <listdirs>     - whether to generate a callback for directory names
//       <cb>           - ptr to the callback
//
//    Returns number of items encountered
// ----------------------------------------------------------------------------

int udflist( dvd_reader_t *device, const char *filename,
   int recursive, int listdirs, udflist_cb_t cb );


#ifdef __cplusplus
}

// c++ wrapper. Probably overkill, but attempts to resolve derived class
// instantiation using the dvd_reader_t pointer

using namespace std;
#include <vector>

class udfLister
{
public:
   // udfItem - derived class "callback", for args see udflist_cb_t above.
   virtual int udfItem( const char *fname, uint16_t ftype, uint32_t lb,
      uint32_t len ) = 0;
   
   // udfList - lists items under <path>, for args see udflist above.
   //    e.g. udfList( myReader, this, "/some/path" );
   static int udfList( dvd_reader_t *reader, udfLister *instance,
      const char *path="", int recursive=true, int listdirs=false )
   {
      udfInstance( reader, instance );
      return ::udflist( reader, path, recursive, listdirs, &udflist_cb );
      udfInstance( reader, instance, false );
   }

private:
   
   typedef struct{ dvd_reader_t *reader; udfLister *instance; } udfkey;

   static udfLister* udfInstance(
      dvd_reader_t *reader, udfLister *instance = NULL, bool alive = true )
   {
      static vector<udfkey> xref;
      
      if( instance )
      {
         xref.push_back( (udfkey){ reader, instance } );
         return instance;
      }
      
      for( int i=0; i < xref.size(); i++ )
         if( xref[i].reader == reader )
         {
            if( alive )
               return xref[i].instance;
            else
               xref.erase( xref.begin() + i );
         }
      return NULL;
   }

   static int udflist_cb( const char *fname, uint16_t ftype, uint32_t lb, uint32_t len, dvd_reader_t *reader )
   {
      if( udfLister *instance = udfInstance( reader ) )
         return instance->udfItem( fname, ftype, lb, len );
      return 0;
   }
};

#endif

#endif

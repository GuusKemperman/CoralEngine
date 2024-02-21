/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2022, assimp team

All rights reserved.

Redistribution and use of this software in source and binary forms,
with or without modification, are permitted provided that the following
conditions are met:

* Redistributions of source code must retain the above
copyright notice, this list of conditions and the
following disclaimer.

* Redistributions in binary form must reproduce the above
copyright notice, this list of conditions and the
following disclaimer in the documentation and/or other
materials provided with the distribution.

* Neither the name of the assimp team, nor the names of its
contributors may be used to endorse or promote products
derived from this software without specific prior
written permission of the assimp team.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
---------------------------------------------------------------------------
*/

/** @file  cexport.h
*  @brief Defines the C-API for the Assimp export interface
*/
#pragma once
#ifndef AI_EXPORT_H_INC
#define AI_EXPORT_H_INC

#ifdef __GNUC__
#pragma GCC system_header
#endif

#ifndef ASSIMP_BUILD_NO_EXPORT

#include <assimp/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct ai***REMOVED***ne;
struct aiFileIO;

// --------------------------------------------------------------------------------
/**
 *  @brief  Describes an file format which Assimp can export to.
 *
 *  Use #aiGetExportFormatCount() to learn how many export-formats are supported by
 *  the current Assimp-build and #aiGetExportFormatDescription() to retrieve the
 *  description of the export format option.
 */
struct aiExportFormatDesc {
    /// a short string ID to uniquely identify the export format. Use this ID string to
    /// specify which file format you want to export to when calling #aiExport***REMOVED***ne().
    /// Example: "dae" or "obj"
    const char *id;

    /// A short description of the file format to present to users. Useful if you want
    /// to allow the user to select an export format.
    const char *description;

    /// Recommended file extension for the exported file in lower case.
    const char *fileExtension;
};

// --------------------------------------------------------------------------------
/** Returns the number of export file formats available in the current Assimp build.
 * Use aiGetExportFormatDescription() to retrieve infos of a specific export format.
 */
ASSIMP_API size_t aiGetExportFormatCount(void);

// --------------------------------------------------------------------------------
/** Returns a description of the nth export file format. Use #aiGetExportFormatCount()
 * to learn how many export formats are supported. The description must be released by
 * calling aiReleaseExportFormatDescription afterwards.
 * @param pIndex Index of the export format to retrieve information for. Valid range is
 *    0 to #aiGetExportFormatCount()
 * @return A description of that specific export format. NULL if pIndex is out of range.
 */
ASSIMP_API const C_STRUCT aiExportFormatDesc *aiGetExportFormatDescription(size_t pIndex);

// --------------------------------------------------------------------------------
/** Release a description of the nth export file format. Must be returned by
* aiGetExportFormatDescription
* @param desc Pointer to the description
*/
ASSIMP_API void aiReleaseExportFormatDescription(const C_STRUCT aiExportFormatDesc *desc);

// --------------------------------------------------------------------------------
/** Create a modifiable copy of a ***REMOVED***ne.
 *  This is useful to import files via Assimp, change their topology and
 *  export them again. Since the ***REMOVED***ne returned by the various importer functions
 *  is const, a modifiable copy is needed.
 *  @param pIn Valid ***REMOVED***ne to be copied
 *  @param pOut Receives a modifiable copy of the ***REMOVED***ne. Use aiFree***REMOVED***ne() to
 *    delete it again.
 */
ASSIMP_API void aiCopy***REMOVED***ne(const C_STRUCT ai***REMOVED***ne *pIn,
        C_STRUCT ai***REMOVED***ne **pOut);

// --------------------------------------------------------------------------------
/** Frees a ***REMOVED***ne copy created using aiCopy***REMOVED***ne() */
ASSIMP_API void aiFree***REMOVED***ne(const C_STRUCT ai***REMOVED***ne *pIn);

// --------------------------------------------------------------------------------
/** Exports the given ***REMOVED***ne to a chosen file format and writes the result file(s) to disk.
* @param p***REMOVED***ne The ***REMOVED***ne to export. Stays in possession of the caller, is not changed by the function.
*   The ***REMOVED***ne is expected to conform to Assimp's Importer output format as specified
*   in the @link data Data Structures Page @endlink. In short, this means the model data
*   should use a right-handed coordinate systems, face winding should be counter-clockwise
*   and the UV coordinate origin is assumed to be in the upper left. If your input data
*   uses different conventions, have a look at the last parameter.
* @param pFormatId ID string to specify to which format you want to export to. Use
* aiGetExportFormatCount() / aiGetExportFormatDescription() to learn which export formats are available.
* @param pFileName Output file to write
* @param pPreprocessing Accepts any choice of the #aiPostProcessSteps enumerated
*   flags, but in reality only a subset of them makes sense here. Specifying
*   'preprocessing' flags is useful if the input ***REMOVED***ne does not conform to
*   Assimp's default conventions as specified in the @link data Data Structures Page @endlink.
*   In short, this means the geometry data should use a right-handed coordinate systems, face
*   winding should be counter-clockwise and the UV coordinate origin is assumed to be in
*   the upper left. The #aiProcess_MakeLeftHanded, #aiProcess_FlipUVs and
*   #aiProcess_FlipWindingOrder flags are used in the import side to allow users
*   to have those defaults automatically adapted to their conventions. Specifying those flags
*   for exporting has the opposite effect, respectively. Some other of the
*   #aiPostProcessSteps enumerated values may be useful as well, but you'll need
*   to try out what their effect on the exported file is. Many formats impose
*   their own restrictions on the structure of the geometry stored therein,
*   so some preprocessing may have little or no effect at all, or may be
*   redundant as exporters would apply them anyhow. A good example
*   is triangulation - whilst you can enforce it by specifying
*   the #aiProcess_Triangulate flag, most export formats support only
*   triangulate data so they would run the step anyway.
*
*   If assimp detects that the input ***REMOVED***ne was directly taken from the importer side of
*   the library (i.e. not copied using aiCopy***REMOVED***ne and potentially modified afterwards),
*   any post-processing steps already applied to the ***REMOVED***ne will not be applied again, unless
*   they show non-idempotent behavior (#aiProcess_MakeLeftHanded, #aiProcess_FlipUVs and
*   #aiProcess_FlipWindingOrder).
* @return a status code indicating the result of the export
* @note Use aiCopy***REMOVED***ne() to get a modifiable copy of a previously
*   imported ***REMOVED***ne.
*/
ASSIMP_API aiReturn aiExport***REMOVED***ne(const C_STRUCT ai***REMOVED***ne *p***REMOVED***ne,
        const char *pFormatId,
        const char *pFileName,
        unsigned int pPreprocessing);

// --------------------------------------------------------------------------------
/** Exports the given ***REMOVED***ne to a chosen file format using custom IO logic supplied by you.
* @param p***REMOVED***ne The ***REMOVED***ne to export. Stays in possession of the caller, is not changed by the function.
* @param pFormatId ID string to specify to which format you want to export to. Use
* aiGetExportFormatCount() / aiGetExportFormatDescription() to learn which export formats are available.
* @param pFileName Output file to write
* @param pIO custom IO implementation to be used. Use this if you use your own storage methods.
*   If none is supplied, a default implementation using standard file IO is used. Note that
*   #aiExport***REMOVED***neToBlob is provided as convenience function to export to memory buffers.
* @param pPreprocessing Please see the documentation for #aiExport***REMOVED***ne
* @return a status code indicating the result of the export
* @note Include <aiFileIO.h> for the definition of #aiFileIO.
* @note Use aiCopy***REMOVED***ne() to get a modifiable copy of a previously
*   imported ***REMOVED***ne.
*/
ASSIMP_API aiReturn aiExport***REMOVED***neEx(const C_STRUCT ai***REMOVED***ne *p***REMOVED***ne,
        const char *pFormatId,
        const char *pFileName,
        C_STRUCT aiFileIO *pIO,
        unsigned int pPreprocessing);

// --------------------------------------------------------------------------------
/** Describes a blob of exported ***REMOVED***ne data. Use #aiExport***REMOVED***neToBlob() to create a blob containing an
* exported ***REMOVED***ne. The memory referred by this structure is owned by Assimp.
* to free its resources. Don't try to free the memory on your side - it will crash for most build configurations
* due to conflicting heaps.
*
* Blobs can be nested - each blob may reference another blob, which may in turn reference another blob and so on.
* This is used when exporters write more than one output file for a given #ai***REMOVED***ne. See the remarks for
* #aiExportDataBlob::name for more information.
*/
struct aiExportDataBlob {
    /// Size of the data in bytes
    size_t size;

    /// The data.
    void *data;

    /** Name of the blob. An empty string always
      * indicates the first (and primary) blob,
      * which contains the actual file data.
      * Any other blobs are auxiliary files produced
      * by exporters (i.e. material files). Existence
      * of such files depends on the file format. Most
      * formats don't split assets across multiple files.
      *
      * If used, blob names usually contain the file
      * extension that should be used when writing
      * the data to disc.
      *
      * The blob names generated can be influenced by
      * setting the #AI_CONFIG_EXPORT_BLOB_NAME export
      * property to the name that is used for the master
      * blob. All other names are typically derived from
      * the base name, by the file format exporter.
     */
    C_STRUCT aiString name;

    /** Pointer to the next blob in the chain or NULL if there is none. */
    C_STRUCT aiExportDataBlob *next;

#ifdef __cplusplus
    /// Default constructor
    aiExportDataBlob() {
        size = 0;
        data = next = nullptr;
    }
    /// Releases the data
    ~aiExportDataBlob() {
        delete[] static_cast<unsigned char *>(data);
        delete next;
    }

    aiExportDataBlob(const aiExportDataBlob &) = delete;
    aiExportDataBlob &operator=(const aiExportDataBlob &) = delete;

#endif // __cplusplus
};

// --------------------------------------------------------------------------------
/** Exports the given ***REMOVED***ne to a chosen file format. Returns the exported data as a binary blob which
* you can write into a file or something. When you're done with the data, use #aiReleaseExportBlob()
* to free the resources associated with the export.
* @param p***REMOVED***ne The ***REMOVED***ne to export. Stays in possession of the caller, is not changed by the function.
* @param pFormatId ID string to specify to which format you want to export to. Use
* #aiGetExportFormatCount() / #aiGetExportFormatDescription() to learn which export formats are available.
* @param pPreprocessing Please see the documentation for #aiExport***REMOVED***ne
* @return the exported data or NULL in case of error
*/
ASSIMP_API const C_STRUCT aiExportDataBlob *aiExport***REMOVED***neToBlob(const C_STRUCT ai***REMOVED***ne *p***REMOVED***ne, const char *pFormatId,
        unsigned int pPreprocessing);

// --------------------------------------------------------------------------------
/** Releases the memory associated with the given exported data. Use this function to free a data blob
* returned by aiExport***REMOVED***ne().
* @param pData the data blob returned by #aiExport***REMOVED***neToBlob
*/
ASSIMP_API void aiReleaseExportBlob(const C_STRUCT aiExportDataBlob *pData);

#ifdef __cplusplus
}
#endif

#endif // ASSIMP_BUILD_NO_EXPORT
#endif // AI_EXPORT_H_INC

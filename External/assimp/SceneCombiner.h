/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2022, assimp team


All rights reserved.

Redistribution and use of this software in source and binary forms,
with or without modification, are permitted provided that the
following conditions are met:

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

----------------------------------------------------------------------
*/

/** @file Declares a helper class, "***REMOVED***neCombiner" providing various
 *  utilities to merge ***REMOVED***nes.
 */
#pragma once
#ifndef AI_SCENE_COMBINER_H_INC
#define AI_SCENE_COMBINER_H_INC

#ifdef __GNUC__
#pragma GCC system_header
#endif

#include <assimp/ai_assert.h>
#include <assimp/types.h>

#include <cstddef>
#include <cstdint>
#include <list>
#include <set>
#include <vector>

struct ai***REMOVED***ne;
struct aiNode;
struct aiMaterial;
struct aiTexture;
struct aiCamera;
struct aiLight;
struct aiMetadata;
struct aiBone;
struct aiMesh;
struct aiAnimMesh;
struct aiAnimation;
struct aiNodeAnim;
struct aiMeshMorphAnim;

namespace Assimp {

// ---------------------------------------------------------------------------
/** \brief Helper data structure for ***REMOVED***neCombiner.
 *
 *  Describes to which node a ***REMOVED***ne must be attached to.
 */
struct AttachmentInfo {
    AttachmentInfo() :
            ***REMOVED***ne(nullptr),
            attachToNode(nullptr) {}

    AttachmentInfo(ai***REMOVED***ne *_***REMOVED***ne, aiNode *_attachToNode) :
            ***REMOVED***ne(_***REMOVED***ne), attachToNode(_attachToNode) {}

    ai***REMOVED***ne ****REMOVED***ne;
    aiNode *attachToNode;
};

// ---------------------------------------------------------------------------
struct NodeAttachmentInfo {
    NodeAttachmentInfo() :
            node(nullptr),
            attachToNode(nullptr),
            resolved(false),
            src_idx(SIZE_MAX) {}

    NodeAttachmentInfo(aiNode *_***REMOVED***ne, aiNode *_attachToNode, size_t idx) :
            node(_***REMOVED***ne), attachToNode(_attachToNode), resolved(false), src_idx(idx) {}

    aiNode *node;
    aiNode *attachToNode;
    bool resolved;
    size_t src_idx;
};

// ---------------------------------------------------------------------------
/** @def AI_INT_MERGE_SCENE_GEN_UNIQUE_NAMES
 *  Generate unique names for all named ***REMOVED***ne items
 */
#define AI_INT_MERGE_SCENE_GEN_UNIQUE_NAMES 0x1

/** @def AI_INT_MERGE_SCENE_GEN_UNIQUE_MATNAMES
 *  Generate unique names for materials, too.
 *  This is not absolutely required to pass the validation.
 */
#define AI_INT_MERGE_SCENE_GEN_UNIQUE_MATNAMES 0x2

/** @def AI_INT_MERGE_SCENE_DUPLICATES_DEEP_CPY
 * Use deep copies of duplicate ***REMOVED***nes
 */
#define AI_INT_MERGE_SCENE_DUPLICATES_DEEP_CPY 0x4

/** @def AI_INT_MERGE_SCENE_RESOLVE_CROSS_ATTACHMENTS
 * If attachment nodes are not found in the given master ***REMOVED***ne,
 * search the other imported ***REMOVED***nes for them in an any order.
 */
#define AI_INT_MERGE_SCENE_RESOLVE_CROSS_ATTACHMENTS 0x8

/** @def AI_INT_MERGE_SCENE_GEN_UNIQUE_NAMES_IF_NECESSARY
 * Can be combined with AI_INT_MERGE_SCENE_GEN_UNIQUE_NAMES.
 * Unique names are generated, but only if this is absolutely
 * required to avoid name conflicts.
 */
#define AI_INT_MERGE_SCENE_GEN_UNIQUE_NAMES_IF_NECESSARY 0x10

typedef std::pair<aiBone *, unsigned int> BoneSrcIndex;

// ---------------------------------------------------------------------------
/** @brief Helper data structure for ***REMOVED***neCombiner::MergeBones.
 */
struct BoneWithHash : public std::pair<uint32_t, aiString *> {
    std::vector<BoneSrcIndex> pSrcBones;
};

// ---------------------------------------------------------------------------
/** @brief Utility for ***REMOVED***neCombiner
 */
struct ***REMOVED***neHelper {
    ***REMOVED***neHelper() :
            ***REMOVED***ne(nullptr),
            idlen(0) {
        id[0] = 0;
    }

    explicit ***REMOVED***neHelper(ai***REMOVED***ne *_***REMOVED***ne) :
            ***REMOVED***ne(_***REMOVED***ne), idlen(0) {
        id[0] = 0;
    }

    AI_FORCE_INLINE ai***REMOVED***ne *operator->() const {
        return ***REMOVED***ne;
    }

    // ***REMOVED***ne we're working on
    ai***REMOVED***ne ****REMOVED***ne;

    // prefix to be added to all identifiers in the ***REMOVED***ne ...
    char id[32];

    // and its strlen()
    unsigned int idlen;

    // hash table to quickly check whether a name is contained in the ***REMOVED***ne
    std::set<unsigned int> hashes;
};

// ---------------------------------------------------------------------------
/** \brief Static helper class providing various utilities to merge two
 *    ***REMOVED***nes. It is intended as internal utility and NOT for use by
 *    applications.
 *
 * The class is currently being used by various postprocessing steps
 * and loaders (ie. LWS).
 */
class ASSIMP_API ***REMOVED***neCombiner {
    // class cannot be instanced
    ***REMOVED***neCombiner() {
        // empty
    }

    ~***REMOVED***neCombiner() {
        // empty
    }

public:
    // -------------------------------------------------------------------
    /** Merges two or more ***REMOVED***nes.
     *
     *  @param dest  Receives a pointer to the destination ***REMOVED***ne. If the
     *    pointer doesn't point to nullptr when the function is called, the
     *    existing ***REMOVED***ne is cleared and refilled.
     *  @param src Non-empty list of ***REMOVED***nes to be merged. The function
     *    deletes the input ***REMOVED***nes afterwards. There may be duplicate ***REMOVED***nes.
     *  @param flags Combination of the AI_INT_MERGE_SCENE flags defined above
     */
    static void Merge***REMOVED***nes(ai***REMOVED***ne **dest, std::vector<ai***REMOVED***ne *> &src,
            unsigned int flags = 0);

    // -------------------------------------------------------------------
    /** Merges two or more ***REMOVED***nes and attaches all ***REMOVED***nes to a specific
     *  position in the node graph of the master ***REMOVED***ne.
     *
     *  @param dest Receives a pointer to the destination ***REMOVED***ne. If the
     *    pointer doesn't point to nullptr when the function is called, the
     *    existing ***REMOVED***ne is cleared and refilled.
     *  @param master Master ***REMOVED***ne. It will be deleted afterwards. All
     *    other ***REMOVED***nes will be inserted in its node graph.
     *  @param src Non-empty list of ***REMOVED***nes to be merged along with their
     *    corresponding attachment points in the master ***REMOVED***ne. The function
     *    deletes the input ***REMOVED***nes afterwards. There may be duplicate ***REMOVED***nes.
     *  @param flags Combination of the AI_INT_MERGE_SCENE flags defined above
     */
    static void Merge***REMOVED***nes(ai***REMOVED***ne **dest, ai***REMOVED***ne *master,
            std::vector<AttachmentInfo> &src,
            unsigned int flags = 0);

    // -------------------------------------------------------------------
    /** Merges two or more meshes
     *
     *  The meshes should have equal vertex formats. Only components
     *  that are provided by ALL meshes will be present in the output mesh.
     *  An exception is made for VColors - they are set to black. The
     *  meshes should have the same material indices, too. The output
     *  material index is always the material index of the first mesh.
     *
     *  @param dest Destination mesh. Must be empty.
     *  @param flags Currently no parameters
     *  @param begin First mesh to be processed
     *  @param end Points to the mesh after the last mesh to be processed
     */
    static void MergeMeshes(aiMesh **dest, unsigned int flags,
            std::vector<aiMesh *>::const_iterator begin,
            std::vector<aiMesh *>::const_iterator end);

    // -------------------------------------------------------------------
    /** Merges two or more bones
     *
     *  @param out Mesh to receive the output bone list
     *  @param flags Currently no parameters
     *  @param begin First mesh to be processed
     *  @param end Points to the mesh after the last mesh to be processed
     */
    static void MergeBones(aiMesh *out, std::vector<aiMesh *>::const_iterator it,
            std::vector<aiMesh *>::const_iterator end);

    // -------------------------------------------------------------------
    /** Merges two or more materials
     *
     *  The materials should be complementary as much as possible. In case
     *  of a property present in different materials, the first occurrence
     *  is used.
     *
     *  @param dest Destination material. Must be empty.
     *  @param begin First material to be processed
     *  @param end Points to the material after the last material to be processed
     */
    static void MergeMaterials(aiMaterial **dest,
            std::vector<aiMaterial *>::const_iterator begin,
            std::vector<aiMaterial *>::const_iterator end);

    // -------------------------------------------------------------------
    /** Builds a list of uniquely named bones in a mesh list
     *
     *  @param asBones Receives the output list
     *  @param it First mesh to be processed
     *  @param end Last mesh to be processed
     */
    static void BuildUniqueBoneList(std::list<BoneWithHash> &asBones,
            std::vector<aiMesh *>::const_iterator it,
            std::vector<aiMesh *>::const_iterator end);

    // -------------------------------------------------------------------
    /** Add a name prefix to all nodes in a ***REMOVED***ne.
     *
     *  @param Current node. This function is called recursively.
     *  @param prefix Prefix to be added to all nodes
     *  @param len STring length
     */
    static void AddNodePrefixes(aiNode *node, const char *prefix,
            unsigned int len);

    // -------------------------------------------------------------------
    /** Add an offset to all mesh indices in a node graph
     *
     *  @param Current node. This function is called recursively.
     *  @param offset Offset to be added to all mesh indices
     */
    static void OffsetNodeMeshIndices(aiNode *node, unsigned int offset);

    // -------------------------------------------------------------------
    /** Attach a list of node graphs to well-defined nodes in a master
     *  graph. This is a helper for Merge***REMOVED***nes()
     *
     *  @param master Master ***REMOVED***ne
     *  @param srcList List of source ***REMOVED***nes along with their attachment
     *    points. If an attachment point is nullptr (or does not exist in
     *    the master graph), a ***REMOVED***ne is attached to the root of the master
     *    graph (as an additional child node)
     *  @duplicates List of duplicates. If elem[n] == n the ***REMOVED***ne is not
     *    a duplicate. Otherwise elem[n] links ***REMOVED***ne n to its first occurrence.
     */
    static void AttachToGraph(ai***REMOVED***ne *master,
            std::vector<NodeAttachmentInfo> &srcList);

    static void AttachToGraph(aiNode *attach,
            std::vector<NodeAttachmentInfo> &srcList);

    // -------------------------------------------------------------------
    /** Get a deep copy of a ***REMOVED***ne
     *
     *  @param dest Receives a pointer to the destination ***REMOVED***ne
     *  @param src Source ***REMOVED***ne - remains unmodified.
     */
    static void Copy***REMOVED***ne(ai***REMOVED***ne **dest, const ai***REMOVED***ne *source, bool allocate = true);

    // -------------------------------------------------------------------
    /** Get a flat copy of a ***REMOVED***ne
     *
     *  Only the first hierarchy layer is copied. All pointer members of
     *  ai***REMOVED***ne are shared by source and destination ***REMOVED***ne.  If the
     *    pointer doesn't point to nullptr when the function is called, the
     *    existing ***REMOVED***ne is cleared and refilled.
     *  @param dest Receives a pointer to the destination ***REMOVED***ne
     *  @param src Source ***REMOVED***ne - remains unmodified.
     */
    static void Copy***REMOVED***neFlat(ai***REMOVED***ne **dest, const ai***REMOVED***ne *source);

    // -------------------------------------------------------------------
    /** Get a deep copy of a mesh
     *
     *  @param dest Receives a pointer to the destination mesh
     *  @param src Source mesh - remains unmodified.
     */
    static void Copy(aiMesh **dest, const aiMesh *src);

    // similar to Copy():
    static void Copy(aiAnimMesh **dest, const aiAnimMesh *src);
    static void Copy(aiMaterial **dest, const aiMaterial *src);
    static void Copy(aiTexture **dest, const aiTexture *src);
    static void Copy(aiAnimation **dest, const aiAnimation *src);
    static void Copy(aiCamera **dest, const aiCamera *src);
    static void Copy(aiBone **dest, const aiBone *src);
    static void Copy(aiLight **dest, const aiLight *src);
    static void Copy(aiNodeAnim **dest, const aiNodeAnim *src);
    static void Copy(aiMeshMorphAnim **dest, const aiMeshMorphAnim *src);
    static void Copy(aiMetadata **dest, const aiMetadata *src);
    static void Copy(aiString **dest, const aiString *src);

    // recursive, of course
    static void Copy(aiNode **dest, const aiNode *src);

private:
    // -------------------------------------------------------------------
    // Same as AddNodePrefixes, but with an additional check
    static void AddNodePrefixesChecked(aiNode *node, const char *prefix,
            unsigned int len,
            std::vector<***REMOVED***neHelper> &input,
            unsigned int cur);

    // -------------------------------------------------------------------
    // Add node identifiers to a hashing set
    static void AddNodeHashes(aiNode *node, std::set<unsigned int> &hashes);

    // -------------------------------------------------------------------
    // Search for duplicate names
    static bool FindNameMatch(const aiString &name,
            std::vector<***REMOVED***neHelper> &input, unsigned int cur);
};

} // namespace Assimp

#endif // !! AI_SCENE_COMBINER_H_INC

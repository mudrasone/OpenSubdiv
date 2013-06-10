//
//     Copyright (C) Pixar. All rights reserved.
//
//     This license governs use of the accompanying software. If you
//     use the software, you accept this license. If you do not accept
//     the license, do not use the software.
//
//     1. Definitions
//     The terms "reproduce," "reproduction," "derivative works," and
//     "distribution" have the same meaning here as under U.S.
//     copyright law.  A "contribution" is the original software, or
//     any additions or changes to the software.
//     A "contributor" is any person or entity that distributes its
//     contribution under this license.
//     "Licensed patents" are a contributor's patent claims that read
//     directly on its contribution.
//
//     2. Grant of Rights
//     (A) Copyright Grant- Subject to the terms of this license,
//     including the license conditions and limitations in section 3,
//     each contributor grants you a non-exclusive, worldwide,
//     royalty-free copyright license to reproduce its contribution,
//     prepare derivative works of its contribution, and distribute
//     its contribution or any derivative works that you create.
//     (B) Patent Grant- Subject to the terms of this license,
//     including the license conditions and limitations in section 3,
//     each contributor grants you a non-exclusive, worldwide,
//     royalty-free license under its licensed patents to make, have
//     made, use, sell, offer for sale, import, and/or otherwise
//     dispose of its contribution in the software or derivative works
//     of the contribution in the software.
//
//     3. Conditions and Limitations
//     (A) No Trademark License- This license does not grant you
//     rights to use any contributor's name, logo, or trademarks.
//     (B) If you bring a patent claim against any contributor over
//     patents that you claim are infringed by the software, your
//     patent license from such contributor to the software ends
//     automatically.
//     (C) If you distribute any portion of the software, you must
//     retain all copyright, patent, trademark, and attribution
//     notices that are present in the software.
//     (D) If you distribute any portion of the software in source
//     code form, you may do so only under this license by including a
//     complete copy of this license with your distribution. If you
//     distribute any portion of the software in compiled or object
//     code form, you may only do so under a license that complies
//     with this license.
//     (E) The software is licensed "as-is." You bear the risk of
//     using it. The contributors give no express warranties,
//     guarantees or conditions. You may have additional consumer
//     rights under your local laws which this license cannot change.
//     To the extent permitted under your local laws, the contributors
//     exclude the implied warranties of merchantability, fitness for
//     a particular purpose and non-infringement.
//
#ifndef OSD_GLSL_TRANSFORM_FEEDBACK_COMPUTE_CONTEXT_H
#define OSD_GLSL_TRANSFORM_FEEDBACK_COMPUTE_CONTEXT_H

#include "../version.h"

#include "../far/vertexEditTables.h"
#include "../osd/vertex.h"
#include "../osd/vertexDescriptor.h"
#include "../osd/nonCopyable.h"

#include "../osd/opengl.h"

#include <vector>

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

class OsdGLSLTransformFeedbackKernelBundle;

class OsdGLSLTransformFeedbackTable : OsdNonCopyable<OsdGLSLTransformFeedbackTable> {
public:
    template<typename T>
    OsdGLSLTransformFeedbackTable(const std::vector<T> &table, GLenum type) {
        createTextureBuffer(table.size() * sizeof(unsigned int), &table[0], type);
    }

    virtual ~OsdGLSLTransformFeedbackTable();

    GLuint GetTexture() const;

private:
    void createTextureBuffer(size_t size, const void *ptr, GLenum type);

    GLuint _devicePtr, 
           _texture;
};

class OsdGLSLTransformFeedbackHEditTable : OsdNonCopyable<OsdGLSLTransformFeedbackHEditTable> {
public:
    OsdGLSLTransformFeedbackHEditTable(const FarVertexEditTables<OsdVertex>::
                      VertexEditBatch &batch);

    virtual ~OsdGLSLTransformFeedbackHEditTable();

    const OsdGLSLTransformFeedbackTable * GetPrimvarIndices() const;

    const OsdGLSLTransformFeedbackTable * GetEditValues() const;

    int GetOperation() const;

    int GetPrimvarOffset() const;

    int GetPrimvarWidth() const;

private:
    OsdGLSLTransformFeedbackTable *_primvarIndicesTable;
    OsdGLSLTransformFeedbackTable *_editValuesTable;

    int _operation;
    int _primvarOffset;
    int _primvarWidth;
};

///
/// \brief GLSL (transform-feedback) Refine Context
///
/// The GLSL (transform-feedback) implementation of the Refine module contextual functionality. 
///
/// Contexts interface the serialized topological data pertaining to the 
/// geometric primitives with the capabilities of the selected discrete 
/// compute device.
///
class OsdGLSLTransformFeedbackComputeContext {
public:
    /// Creates an OsdGLSLTransformFeedbackComputeContext instance
    ///
    /// @param farmesh the FarMesh used for this Context.
    ///
    static OsdGLSLTransformFeedbackComputeContext * Create(FarMesh<OsdVertex> const *farmesh);

    /// Destructor
    virtual ~OsdGLSLTransformFeedbackComputeContext();

    /// Binds a vertex and a varying data buffers to the context. Binding ensures
    /// that data buffers are properly inter-operated between Contexts and 
    /// Controllers operating across multiple devices.
    ///
    /// @param vertex   a buffer containing vertex-interpolated primvar data
    ///
    /// @param varying  a buffer containing varying-interpolated primvar data
    ///
    template<class VERTEX_BUFFER, class VARYING_BUFFER>
    void Bind(VERTEX_BUFFER *vertex, VARYING_BUFFER *varying) {

        _currentVertexBuffer = vertex ? vertex->BindVBO() : 0;
        _currentVaryingBuffer = varying ? varying->BindVBO() : 0;

        _vdesc.numVertexElements = vertex ? vertex->GetNumElements() : 0;
        _vdesc.numVaryingElements = varying ? varying->GetNumElements() : 0;

        bindTextures();
    }

    /// Unbinds any previously bound vertex and varying data buffers.
    void Unbind() {
        _currentVertexBuffer = 0;
        _currentVaryingBuffer = 0;
        unbindTextures();
    }

    /// Returns one of the vertex refinement tables.
    ///
    /// @param tableIndex the type of table
    ///
    const OsdGLSLTransformFeedbackTable * GetTable(int tableIndex) const;

    /// Returns the number of hierarchical edit tables
    int GetNumEditTables() const;

    /// Returns a specific hierarchical edit table
    ///
    /// @param tableIndex the index of the table
    ///
    const OsdGLSLTransformFeedbackHEditTable * GetEditTable(int tableIndex) const;

    /// Returns a handle to the vertex-interpolated buffer
    GLuint GetCurrentVertexBuffer() const;

    /// Returns a handle to the varying-interpolated buffer
    GLuint GetCurrentVaryingBuffer() const;

    /// Returns an OsdVertexDescriptor if vertex buffers have been bound.
    ///
    /// @return a descriptor for the format of the vertex data currently bound
    ///
    OsdVertexDescriptor const & GetVertexDescriptor() const {
        return _vdesc;
    }

    OsdGLSLTransformFeedbackKernelBundle * GetKernelBundle() const;

    void SetKernelBundle(OsdGLSLTransformFeedbackKernelBundle *kernelBundle);

    void BindEditTextures(int editIndex);

    void UnbindEditTextures();

protected:
    explicit OsdGLSLTransformFeedbackComputeContext(FarMesh<OsdVertex> const *farMesh);

    void bindTexture(GLuint sampler, GLuint texture, int unit);

    void unbindTexture(GLuint unit);

    void bindTextures();

    void unbindTextures();

private:
    std::vector<OsdGLSLTransformFeedbackTable*> _tables;
    std::vector<OsdGLSLTransformFeedbackHEditTable*> _editTables;

    GLuint _vertexTexture,
           _varyingTexture;

    OsdVertexDescriptor _vdesc;

    GLuint _currentVertexBuffer, 
           _currentVaryingBuffer;

    OsdGLSLTransformFeedbackKernelBundle * _kernelBundle;
};

}  // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;

}  // end namespace OpenSubdiv

#endif  // OSD_GLSL_TRANSFORM_FEEDBACK_COMPUTE_CONTEXT_H

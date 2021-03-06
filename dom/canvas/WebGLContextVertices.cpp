/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "WebGLContext.h"

#include "GLContext.h"
#include "mozilla/CheckedInt.h"
#include "WebGLBuffer.h"
#include "WebGLFramebuffer.h"
#include "WebGLProgram.h"
#include "WebGLRenderbuffer.h"
#include "WebGLShader.h"
#include "WebGLTexture.h"
#include "WebGLVertexArray.h"
#include "WebGLVertexAttribData.h"

#include "mozilla/Casting.h"

namespace mozilla {

JSObject*
WebGLContext::GetVertexAttribFloat32Array(JSContext* cx, GLuint index)
{
    GLfloat attrib[4];
    if (index) {
        gl->fGetVertexAttribfv(index, LOCAL_GL_CURRENT_VERTEX_ATTRIB, attrib);
    } else {
        memcpy(attrib, mGenericVertexAttrib0Data, sizeof(mGenericVertexAttrib0Data));
    }
    return dom::Float32Array::Create(cx, this, 4, attrib);
}

JSObject*
WebGLContext::GetVertexAttribInt32Array(JSContext* cx, GLuint index)
{
    GLint attrib[4];
    if (index) {
        gl->fGetVertexAttribIiv(index, LOCAL_GL_CURRENT_VERTEX_ATTRIB, attrib);
    } else {
        memcpy(attrib, mGenericVertexAttrib0Data, sizeof(mGenericVertexAttrib0Data));
    }
    return dom::Int32Array::Create(cx, this, 4, attrib);
}

JSObject*
WebGLContext::GetVertexAttribUint32Array(JSContext* cx, GLuint index)
{
    GLuint attrib[4];
    if (index) {
        gl->fGetVertexAttribIuiv(index, LOCAL_GL_CURRENT_VERTEX_ATTRIB, attrib);
    } else {
        memcpy(attrib, mGenericVertexAttrib0Data, sizeof(mGenericVertexAttrib0Data));
    }
    return dom::Uint32Array::Create(cx, this, 4, attrib);
}

////////////////////////////////////////

void
WebGLContext::VertexAttrib4f(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w,
                             const char* funcName)
{
    if (!funcName) {
        funcName = "vertexAttrib4f";
    }

    if (IsContextLost())
        return;

    if (!ValidateAttribIndex(index, funcName))
        return;

    ////

    gl->MakeCurrent();
    if (index || !gl->IsCompatibilityProfile()) {
        gl->fVertexAttrib4f(index, x, y, z, w);
    }

    ////

    mGenericVertexAttribTypes[index] = LOCAL_GL_FLOAT;

    if (!index) {
        const float data[4] = { x, y, z, w };
        memcpy(mGenericVertexAttrib0Data, data, sizeof(data));
    }
}

void
WebGL2Context::VertexAttribI4i(GLuint index, GLint x, GLint y, GLint z, GLint w,
                               const char* funcName)
{
    if (!funcName) {
        funcName = "vertexAttribI4i";
    }

    if (IsContextLost())
        return;

    if (!ValidateAttribIndex(index, funcName))
        return;

    ////

    gl->MakeCurrent();
    if (index || !gl->IsCompatibilityProfile()) {
        gl->fVertexAttribI4i(index, x, y, z, w);
    }

    ////

    mGenericVertexAttribTypes[index] = LOCAL_GL_INT;

    if (!index) {
        const int32_t data[4] = { x, y, z, w };
        memcpy(mGenericVertexAttrib0Data, data, sizeof(data));
    }
}

void
WebGL2Context::VertexAttribI4ui(GLuint index, GLuint x, GLuint y, GLuint z, GLuint w,
                                const char* funcName)
{
    if (!funcName) {
        funcName = "vertexAttribI4ui";
    }

    if (IsContextLost())
        return;

    if (!ValidateAttribIndex(index, funcName))
        return;

    ////

    gl->MakeCurrent();
    if (index || !gl->IsCompatibilityProfile()) {
        gl->fVertexAttribI4ui(index, x, y, z, w);
    }

    ////

    mGenericVertexAttribTypes[index] = LOCAL_GL_UNSIGNED_INT;

    if (!index) {
        const uint32_t data[4] = { x, y, z, w };
        memcpy(mGenericVertexAttrib0Data, data, sizeof(data));
    }
}

////////////////////////////////////////

void
WebGLContext::EnableVertexAttribArray(GLuint index)
{
    if (IsContextLost())
        return;

    if (!ValidateAttribIndex(index, "enableVertexAttribArray"))
        return;

    MakeContextCurrent();
    InvalidateBufferFetching();

    gl->fEnableVertexAttribArray(index);

    MOZ_ASSERT(mBoundVertexArray);
    mBoundVertexArray->mAttribs[index].mEnabled = true;
}

void
WebGLContext::DisableVertexAttribArray(GLuint index)
{
    if (IsContextLost())
        return;

    if (!ValidateAttribIndex(index, "disableVertexAttribArray"))
        return;

    MakeContextCurrent();
    InvalidateBufferFetching();

    if (index || !gl->IsCompatibilityProfile()) {
        gl->fDisableVertexAttribArray(index);
    }

    MOZ_ASSERT(mBoundVertexArray);
    mBoundVertexArray->mAttribs[index].mEnabled = false;
}

JS::Value
WebGLContext::GetVertexAttrib(JSContext* cx, GLuint index, GLenum pname,
                              ErrorResult& rv)
{
    const char funcName[] = "getVertexAttrib";
    if (IsContextLost())
        return JS::NullValue();

    if (!ValidateAttribIndex(index, funcName))
        return JS::NullValue();

    MOZ_ASSERT(mBoundVertexArray);

    MakeContextCurrent();

    switch (pname) {
    case LOCAL_GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING:
        return WebGLObjectAsJSValue(cx, mBoundVertexArray->mAttribs[index].mBuf.get(), rv);

    case LOCAL_GL_VERTEX_ATTRIB_ARRAY_STRIDE:
        return JS::Int32Value(mBoundVertexArray->mAttribs[index].Stride());

    case LOCAL_GL_VERTEX_ATTRIB_ARRAY_SIZE:
        return JS::Int32Value(mBoundVertexArray->mAttribs[index].Size());

    case LOCAL_GL_VERTEX_ATTRIB_ARRAY_TYPE:
        return JS::Int32Value(mBoundVertexArray->mAttribs[index].Type());

    case LOCAL_GL_VERTEX_ATTRIB_ARRAY_INTEGER:
        if (IsWebGL2())
            return JS::BooleanValue(mBoundVertexArray->mAttribs[index].IntegerFunc());

        break;

    case LOCAL_GL_VERTEX_ATTRIB_ARRAY_DIVISOR:
        if (IsWebGL2() ||
            IsExtensionEnabled(WebGLExtensionID::ANGLE_instanced_arrays))
        {
            return JS::Int32Value(mBoundVertexArray->mAttribs[index].mDivisor);
        }
        break;

    case LOCAL_GL_CURRENT_VERTEX_ATTRIB:
        {
            JS::RootedObject obj(cx);
            switch (mGenericVertexAttribTypes[index]) {
            case LOCAL_GL_FLOAT:
                obj = GetVertexAttribFloat32Array(cx, index);
                break;

            case LOCAL_GL_INT:
                obj =  GetVertexAttribInt32Array(cx, index);
                break;

            case LOCAL_GL_UNSIGNED_INT:
                obj = GetVertexAttribUint32Array(cx, index);
                break;
            }

            if (!obj)
                rv.Throw(NS_ERROR_OUT_OF_MEMORY);
            return JS::ObjectOrNullValue(obj);
        }

    case LOCAL_GL_VERTEX_ATTRIB_ARRAY_ENABLED:
        return JS::BooleanValue(mBoundVertexArray->mAttribs[index].mEnabled);

    case LOCAL_GL_VERTEX_ATTRIB_ARRAY_NORMALIZED:
        return JS::BooleanValue(mBoundVertexArray->mAttribs[index].Normalized());

    default:
        break;
    }

    ErrorInvalidEnumInfo("getVertexAttrib: parameter", pname);
    return JS::NullValue();
}

WebGLsizeiptr
WebGLContext::GetVertexAttribOffset(GLuint index, GLenum pname)
{
    if (IsContextLost())
        return 0;

    if (!ValidateAttribIndex(index, "getVertexAttribOffset"))
        return 0;

    if (pname != LOCAL_GL_VERTEX_ATTRIB_ARRAY_POINTER) {
        ErrorInvalidEnum("getVertexAttribOffset: bad parameter");
        return 0;
    }

    MOZ_ASSERT(mBoundVertexArray);
    return mBoundVertexArray->mAttribs[index].ByteOffset();
}

void
WebGLContext::VertexAttribPointer(GLuint index, GLint size, GLenum type,
                                  WebGLboolean normalized, GLsizei stride,
                                  WebGLintptr byteOffset)
{
    if (IsContextLost())
        return;

    if (!ValidateAttribIndex(index, "vertexAttribPointer"))
        return;

    if (!ValidateAttribPointer(false, index, size, type, normalized, stride, byteOffset, "vertexAttribPointer"))
        return;

    MOZ_ASSERT(mBoundVertexArray);

    InvalidateBufferFetching();

    /* XXX make work with bufferSubData & heterogeneous types
     if (type != mBoundArrayBuffer->GLType())
     return ErrorInvalidOperation("vertexAttribPointer: type must match bound VBO type: %d != %d", type, mBoundArrayBuffer->GLType());
     */

    MakeContextCurrent();
    gl->fVertexAttribPointer(index, size, type, normalized, stride,
                             reinterpret_cast<void*>(byteOffset));

    WebGLVertexAttribData& vd = mBoundVertexArray->mAttribs[index];
    const bool integerFunc = false;
    vd.VertexAttribPointer(integerFunc, mBoundArrayBuffer, size, type, normalized, stride,
                           byteOffset);
}

void
WebGLContext::VertexAttribDivisor(GLuint index, GLuint divisor)
{
    if (IsContextLost())
        return;

    if (!ValidateAttribIndex(index, "vertexAttribDivisor"))
        return;

    MOZ_ASSERT(mBoundVertexArray);

    WebGLVertexAttribData& vd = mBoundVertexArray->mAttribs[index];
    vd.mDivisor = divisor;

    InvalidateBufferFetching();

    MakeContextCurrent();

    gl->fVertexAttribDivisor(index, divisor);
}

} // namespace mozilla

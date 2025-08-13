#include "AndroidSurfaceVideoDecoder.h"
#include <cstring>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <android/log.h>

#define LOG_TAG "MRDesk.SurfaceDecoder"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

AndroidSurfaceVideoDecoder::AndroidSurfaceVideoDecoder() {
  LOGD("AndroidSurfaceVideoDecoder created");
}

AndroidSurfaceVideoDecoder::~AndroidSurfaceVideoDecoder() {
  Cleanup();
}

const char* AndroidSurfaceVideoDecoder::GetMimeType(CompressionType type) {
  switch (type) {
    case COMPRESSION_H264:
      return "video/avc";
    case COMPRESSION_H265:
      return "video/hevc";
    case COMPRESSION_AV1:
      return "video/av01";
    default:
      return nullptr;
  }
}

bool AndroidSurfaceVideoDecoder::Initialize(
    uint32_t width, uint32_t height, CompressionType compression) {
  // This method is required by IVideoDecoder but we prefer
  // InitializeWithSurface
  LOGE("Use InitializeWithSurface instead for Surface-based decoding");
  return false;
}

bool AndroidSurfaceVideoDecoder::InitializeWithSurface(
    uint32_t width,
    uint32_t height,
    CompressionType compression,
    ANativeWindow* surface) {
  LOGD(
      "Initializing Surface decoder: %dx%d, compression=%d",
      width,
      height,
      compression);

  if (m_isInitialized) {
    LOGE("Decoder already initialized");
    return false;
  }

  if (!surface) {
    LOGE("Surface is null");
    return false;
  }

  m_width = width;
  m_height = height;
  m_compressionType = compression;
  m_surface = surface;
  m_isShuttingDown = false;

  const char* mimeType = GetMimeType(compression);
  if (!mimeType) {
    LOGE("Unsupported compression type: %d", compression);
    return false;
  }

  // Create decoder for the specified MIME type
  m_codec = AMediaCodec_createDecoderByType(mimeType);
  if (!m_codec) {
    LOGE("Failed to create decoder for MIME type: %s", mimeType);
    return false;
  }

  // Create format
  m_format = AMediaFormat_new();
  AMediaFormat_setString(m_format, AMEDIAFORMAT_KEY_MIME, mimeType);
  AMediaFormat_setInt32(m_format, AMEDIAFORMAT_KEY_WIDTH, width);
  AMediaFormat_setInt32(m_format, AMEDIAFORMAT_KEY_HEIGHT, height);

  // Configure the decoder with the Surface
  media_status_t status =
      AMediaCodec_configure(m_codec, m_format, surface, nullptr, 0);
  if (status != AMEDIA_OK) {
    LOGE("Failed to configure decoder with Surface: %d", status);
    Cleanup();
    return false;
  }

  // Start the decoder
  status = AMediaCodec_start(m_codec);
  if (status != AMEDIA_OK) {
    LOGE("Failed to start decoder: %d", status);
    Cleanup();
    return false;
  }

  m_isInitialized = true;
  LOGD("Surface decoder initialized successfully");
  return true;
}

bool AndroidSurfaceVideoDecoder::CreateSurfaceTexture(JNIEnv* env) {
  // Generate texture for GL_TEXTURE_EXTERNAL_OES
  GLuint textureId;
  glGenTextures(1, &textureId);
  glBindTexture(GL_TEXTURE_EXTERNAL_OES, textureId);

  // Set texture parameters
  glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  // Create SurfaceTexture
  jclass surfaceTextureClass =
      env->FindClass("android/graphics/SurfaceTexture");
  if (!surfaceTextureClass) {
    LOGE("Failed to find SurfaceTexture class");
    return false;
  }

  jmethodID constructor =
      env->GetMethodID(surfaceTextureClass, "<init>", "(I)V");
  if (!constructor) {
    LOGE("Failed to get SurfaceTexture constructor");
    return false;
  }

  jobject surfaceTexture =
      env->NewObject(surfaceTextureClass, constructor, (jint)textureId);
  if (!surfaceTexture) {
    LOGE("Failed to create SurfaceTexture");
    return false;
  }

  m_surfaceTexture = env->NewGlobalRef(surfaceTexture);

  // Create Surface from SurfaceTexture
  jclass surfaceClass = env->FindClass("android/view/Surface");
  if (!surfaceClass) {
    LOGE("Failed to find Surface class");
    return false;
  }

  jmethodID surfaceConstructor = env->GetMethodID(
      surfaceClass, "<init>", "(Landroid/graphics/SurfaceTexture;)V");
  if (!surfaceConstructor) {
    LOGE("Failed to get Surface constructor");
    return false;
  }

  jobject surface =
      env->NewObject(surfaceClass, surfaceConstructor, m_surfaceTexture);
  if (!surface) {
    LOGE("Failed to create Surface");
    return false;
  }

  m_surface_java = env->NewGlobalRef(surface);

  // Get native window from Surface
  m_surface = ANativeWindow_fromSurface(env, m_surface_java);
  if (!m_surface) {
    LOGE("Failed to get ANativeWindow from Surface");
    return false;
  }

  LOGD(
      "SurfaceTexture and Surface created successfully with texture ID: %u",
      textureId);
  return true;
}

bool AndroidSurfaceVideoDecoder::DecodeFrame(
    const uint8_t* compressedData,
    size_t dataSize,
    std::vector<uint8_t>& rgbaData) {
  // For Surface decoding, we don't return pixel data
  // This method is kept for IVideoDecoder compatibility
  LOGD("DecodeFrame called - using Surface decoding, no pixel data returned");
  rgbaData.clear(); // Empty since we're rendering to Surface
  return DecodeFrameToSurface(compressedData, dataSize);
}

bool AndroidSurfaceVideoDecoder::DecodeFrameToSurface(
    const uint8_t* compressedData, size_t dataSize) {
  if (!m_isInitialized || !m_codec || m_isShuttingDown) {
    LOGE("Decoder not initialized or shutting down");
    return false;
  }

  LOGD("Decoding frame to Surface: %zu bytes", dataSize);

  // Get input buffer
  ssize_t inputBufferIndex =
      AMediaCodec_dequeueInputBuffer(m_codec, 10000); // 10ms timeout
  if (inputBufferIndex < 0) {
    LOGE("No input buffer available: %zd", inputBufferIndex);
    return false;
  }

  // Fill input buffer
  size_t inputBufferSize;
  uint8_t* inputBuffer =
      AMediaCodec_getInputBuffer(m_codec, inputBufferIndex, &inputBufferSize);
  if (!inputBuffer || inputBufferSize < dataSize) {
    LOGE("Input buffer too small: %zu < %zu", inputBufferSize, dataSize);
    return false;
  }

  memcpy(inputBuffer, compressedData, dataSize);

  // Queue input buffer
  media_status_t status = AMediaCodec_queueInputBuffer(
      m_codec, inputBufferIndex, 0, dataSize, 0, 0);
  if (status != AMEDIA_OK) {
    LOGE("Failed to queue input buffer: %d", status);
    return false;
  }

  // Get output buffer - for Surface decoding, we just need to dequeue to
  // trigger rendering
  AMediaCodecBufferInfo bufferInfo;
  ssize_t outputBufferIndex;

  // Retry up to 3 times to handle format changes
  for (int retry = 0; retry < 3; retry++) {
    outputBufferIndex =
        AMediaCodec_dequeueOutputBuffer(m_codec, &bufferInfo, 10000);

    if (outputBufferIndex == AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED) {
      LOGD("Output format changed (retry %d/3)", retry + 1);
      continue;
    }

    if (outputBufferIndex == AMEDIACODEC_INFO_OUTPUT_BUFFERS_CHANGED) {
      LOGD("Output buffers changed (retry %d/3)", retry + 1);
      continue;
    }

    break;
  }

  if (outputBufferIndex < 0) {
    LOGE("No output buffer available: %zd", outputBufferIndex);
    return false;
  }

  // For Surface decoding, we don't need to access the buffer data
  // The decoded frame is automatically rendered to the Surface
  // Just release the buffer to indicate we're done with it
  AMediaCodec_releaseOutputBuffer(
      m_codec, outputBufferIndex, true); // true = render to surface

  LOGD("Frame decoded and rendered to Surface successfully");
  return true;
}

uint32_t AndroidSurfaceVideoDecoder::GetTextureId() const {
  // This would need to be stored when creating the SurfaceTexture
  // For now, return 0 - this should be implemented properly
  return 0;
}

void AndroidSurfaceVideoDecoder::UpdateTexImage() {
  if (!m_surfaceTexture) {
    LOGE("SurfaceTexture is null");
    return;
  }

  // Get current JNI environment
  JNIEnv* env = nullptr;
  if (m_javaVM && m_javaVM->GetEnv((void**)&env, JNI_VERSION_1_6) == JNI_OK) {
    jclass surfaceTextureClass = env->GetObjectClass(m_surfaceTexture);
    jmethodID updateTexImageMethod =
        env->GetMethodID(surfaceTextureClass, "updateTexImage", "()V");

    if (updateTexImageMethod) {
      env->CallVoidMethod(m_surfaceTexture, updateTexImageMethod);
      LOGD("SurfaceTexture.updateTexImage() called");
    } else {
      LOGE("Failed to find updateTexImage method");
    }
  } else {
    LOGE("Failed to get JNI environment for updateTexImage");
  }
}

void AndroidSurfaceVideoDecoder::Cleanup() {
  m_isShuttingDown = true;

  if (m_codec) {
    AMediaCodec_stop(m_codec);
    AMediaCodec_delete(m_codec);
    m_codec = nullptr;
  }

  if (m_format) {
    AMediaFormat_delete(m_format);
    m_format = nullptr;
  }

  if (m_surface) {
    ANativeWindow_release(m_surface);
    m_surface = nullptr;
  }

  // Clean up JNI global references
  if (m_javaVM) {
    JNIEnv* env = nullptr;
    if (m_javaVM->GetEnv((void**)&env, JNI_VERSION_1_6) == JNI_OK) {
      if (m_surfaceTexture) {
        env->DeleteGlobalRef(m_surfaceTexture);
        m_surfaceTexture = nullptr;
      }
      if (m_surface_java) {
        env->DeleteGlobalRef(m_surface_java);
        m_surface_java = nullptr;
      }
    }
  }

  m_isInitialized = false;
  LOGD("Surface decoder cleaned up");
}
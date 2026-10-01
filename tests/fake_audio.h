#ifndef FAKE_AUDIO_H
#define FAKE_AUDIO_H

#include "raylib.h"
#include <cassert>
#include <cstdint>
#include <set>
#include <vector>

// A silent backend lets controller tests count cues without needing speakers.
namespace FakeAudio {
inline bool ready = false;
inline bool device_allowed = true;
inline int fail_load_at = -1;
inline int initializations = 0;
inline int closes = 0;
inline int loads = 0;
inline int stops = 0;
inline int unloads = 0;
inline std::set<std::uintptr_t> live;
inline std::set<std::uintptr_t> playing;
inline std::vector<std::uintptr_t> played;
inline std::vector<std::vector<float>> samples;
inline std::vector<float> volumes;
inline std::uintptr_t id(Sound sound) { return reinterpret_cast<std::uintptr_t>(sound.stream.buffer); }
inline void reset() {
  assert(live.empty());
  ready = false; device_allowed = true; fail_load_at = -1;
  initializations = closes = loads = stops = unloads = 0;
  playing.clear(); played.clear(); samples.clear(); volumes.clear();
}
}

bool IsAudioDeviceReady() { return FakeAudio::ready; }
void InitAudioDevice() { ++FakeAudio::initializations; FakeAudio::ready = FakeAudio::device_allowed; }
void CloseAudioDevice() {
  assert(FakeAudio::live.empty());
  ++FakeAudio::closes; FakeAudio::ready = false;
}
Sound LoadSoundFromWave(Wave wave) {
  assert(FakeAudio::ready && wave.sampleSize == 32 && wave.sampleRate == 48000 && wave.channels == 1);
  const auto *data = static_cast<const float *>(wave.data);
  FakeAudio::samples.emplace_back(data, data + wave.frameCount);
  ++FakeAudio::loads;
  Sound sound{};
  if (FakeAudio::loads == FakeAudio::fail_load_at) return sound;
  sound.stream.buffer = reinterpret_cast<rAudioBuffer *>(static_cast<std::uintptr_t>(FakeAudio::loads));
  sound.stream.sampleRate = wave.sampleRate;
  sound.stream.sampleSize = wave.sampleSize;
  sound.stream.channels = wave.channels;
  sound.frameCount = wave.frameCount;
  FakeAudio::live.insert(FakeAudio::id(sound));
  return sound;
}
bool IsSoundValid(Sound sound) { return sound.stream.buffer && sound.frameCount > 0; }
void SetSoundVolume(Sound sound, float volume) {
  assert(FakeAudio::live.contains(FakeAudio::id(sound)));
  FakeAudio::volumes.push_back(volume);
}
void UnloadSound(Sound sound) {
  assert(FakeAudio::live.erase(FakeAudio::id(sound)) == 1);
  FakeAudio::playing.erase(FakeAudio::id(sound));
  ++FakeAudio::unloads;
}
void PlaySound(Sound sound) {
  assert(FakeAudio::ready && FakeAudio::live.contains(FakeAudio::id(sound)));
  FakeAudio::played.push_back(FakeAudio::id(sound));
  FakeAudio::playing.insert(FakeAudio::id(sound));
}
void StopSound(Sound sound) {
  assert(FakeAudio::live.contains(FakeAudio::id(sound)));
  FakeAudio::playing.erase(FakeAudio::id(sound));
  ++FakeAudio::stops;
}
#endif

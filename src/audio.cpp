#include "audio.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <vector>

namespace {
constexpr unsigned int kSampleRate = 48000;
constexpr double kTau = 2 * std::numbers::pi;
constexpr std::array<GameEvent, 4> kCues = {
    GameEvent::Revealed, GameEvent::Flagged, GameEvent::Won, GameEvent::Lost};

float smooth(double value) {
  const double t = std::clamp(value, 0.0, 1.0);
  return static_cast<float>(t * t * (3 - 2 * t));
}
float envelope(double t, double duration, double attack, double release) {
  return smooth(t / attack) * smooth((duration - t) / release);
}

std::vector<float> synthesize(GameEvent cue) {
  const double duration = cue == GameEvent::Revealed ? 0.075 :
      cue == GameEvent::Flagged ? 0.11 : cue == GameEvent::Won ? 0.72 : 0.38;
  std::vector<float> samples(static_cast<std::size_t>(duration * kSampleRate));
  double phase = 0;
  for (std::size_t i = 0; i < samples.size(); ++i) {
    const double t = static_cast<double>(i) / kSampleRate;
    double value = 0;
    if (cue == GameEvent::Revealed) {
      // A soft, woody tick, with a rapid decay and no sharp attack.
      phase += kTau * (1050 - 350 * t / duration) / kSampleRate;
      value = 0.34 * envelope(t, duration, 0.003, 0.018) * std::exp(-t * 52) *
          (std::sin(phase) + 0.18 * std::sin(2.3 * phase));
    } else if (cue == GameEvent::Flagged) {
      // A rounded pop with a falling pitch, shared by flag and unflag.
      phase += kTau * (360 + 650 * std::exp(-t * 45)) / kSampleRate;
      value = 0.32 * envelope(t, duration, 0.005, 0.025) * std::exp(-t * 29) *
          (std::sin(phase) + 0.12 * std::sin(2 * phase));
    } else if (cue == GameEvent::Won) {
      // A brief ascending major arpeggio; the notes gently overlap.
      constexpr double notes[] = {523.25, 659.25, 783.99, 1046.50};
      for (int note = 0; note < 4; ++note) {
        const double age = t - note * 0.085;
        if (age < 0 || age > 0.45) continue;
        value += 0.19 * envelope(age, 0.45, 0.009, 0.12) * std::exp(-age * 8) *
            (std::sin(kTau * notes[note] * age) + 0.13 * std::sin(kTau * notes[note] * 2 * age));
      }
    } else {
      // A soft nasal honk: smooth pitch bend, a few rounded harmonics,
      // and a long release rather than an abrupt game-over buzzer.
      const double u = t / duration;
      const double frequency = 370 + 95 * std::sin(std::numbers::pi * u) - 100 * u + 3 * std::sin(kTau * 5 * t);
      phase += kTau * frequency / kSampleRate;
      value = 0.35 * envelope(t, duration, 0.035, 0.16) *
          (0.55 * std::sin(phase) + 0.28 * std::sin(2 * phase) +
           0.22 * std::sin(3 * phase) + 0.07 * std::sin(5 * phase));
    }
    samples[i] = static_cast<float>(value);
  }
  samples.front() = samples.back() = 0;
  return samples;
}
} // namespace

SoundEffects::SoundEffects() {
  if (!IsAudioDeviceReady()) {
    InitAudioDevice();
    owns_device_ = IsAudioDeviceReady();
  }
  if (!IsAudioDeviceReady()) return;
  for (std::size_t i = 0; i < sounds_.size(); ++i) {
    auto samples = synthesize(kCues[i]);
    const Wave wave{static_cast<unsigned int>(samples.size()), kSampleRate, 32, 1, samples.data()};
    // raylib copies the samples into its playback buffer here.
    sounds_[i] = LoadSoundFromWave(wave);
    if (!IsSoundValid(sounds_[i])) return;
    SetSoundVolume(sounds_[i], 0.45f);
  }
  available_ = true;
}

SoundEffects::~SoundEffects() {
  for (const auto &sound : sounds_) if (IsSoundValid(sound)) UnloadSound(sound);
  if (owns_device_) CloseAudioDevice();
}

void SoundEffects::toggle() {
  if (!available_) return;
  enabled_ = !enabled_;
  if (!enabled_) stop();
}

void SoundEffects::stop() {
  for (const auto &sound : sounds_) if (IsSoundValid(sound)) StopSound(sound);
}

void SoundEffects::play(GameEvent event) {
  if (!enabled()) return;
  const auto found = std::find(kCues.begin(), kCues.end(), event);
  if (found == kCues.end()) return;
  // A result replaces the move's tick/pop, and has no competing cue underneath.
  if (event == GameEvent::Won || event == GameEvent::Lost) stop();
  PlaySound(sounds_[static_cast<std::size_t>(found - kCues.begin())]);
}

#include "audio.h"
#include "fake_audio.h"

#include <algorithm>
#include <cmath>
#include <iostream>

int main() {
  FakeAudio::reset();
  {
    SoundEffects audio;
    assert(audio.available() && audio.enabled());
    assert(FakeAudio::loads == 4);
    for (const auto &samples : FakeAudio::samples) {
      assert(samples.size() >= 2400 && samples.size() < 48000);
      assert(samples.front() == 0 && samples.back() == 0);
      float peak = 0;
      for (std::size_t i = 0; i < samples.size(); ++i) {
        assert(std::isfinite(samples[i]));
        peak = std::max(peak, std::abs(samples[i]));
        if (i > 0) assert(std::abs(samples[i] - samples[i - 1]) < 0.08f);
      }
      assert(peak > 0.05f && peak < 0.65f); // Audible signal, with headroom and smooth edges.
    }
    for (float volume : FakeAudio::volumes) assert(volume > 0 && volume <= 0.5f);
    audio.play(GameEvent::None);
    assert(FakeAudio::played.empty());
    audio.play(GameEvent::Revealed);
    audio.play(GameEvent::Flagged);
    assert((FakeAudio::played == std::vector<std::uintptr_t>{1, 2}));
    audio.play(GameEvent::Won);
    assert((FakeAudio::playing == std::set<std::uintptr_t>{3}));
    audio.play(GameEvent::Lost);
    assert((FakeAudio::playing == std::set<std::uintptr_t>{4}));
    audio.toggle();
    assert(!audio.enabled() && FakeAudio::playing.empty());
    const auto count = FakeAudio::played.size();
    audio.play(GameEvent::Revealed);
    audio.play(GameEvent::Won);
    assert(FakeAudio::played.size() == count);
    audio.toggle();
    audio.play(GameEvent::Flagged);
    assert(audio.enabled() && FakeAudio::played.size() == count + 1);
    audio.stop();
    assert(FakeAudio::playing.empty());
  }
  assert(FakeAudio::unloads == 4 && FakeAudio::closes == 1);

  FakeAudio::reset();
  FakeAudio::device_allowed = false;
  {
    SoundEffects audio;
    assert(!audio.available() && !audio.enabled());
    audio.toggle(); audio.play(GameEvent::Revealed); audio.stop();
    assert(FakeAudio::loads == 0 && FakeAudio::played.empty());
  }
  assert(FakeAudio::initializations == 1 && FakeAudio::closes == 0);

  FakeAudio::reset();
  FakeAudio::fail_load_at = 3;
  {
    SoundEffects audio;
    assert(!audio.available());
    audio.play(GameEvent::Won);
    assert(FakeAudio::played.empty());
  }
  assert(FakeAudio::unloads == 2 && FakeAudio::closes == 1);

  FakeAudio::reset();
  FakeAudio::ready = true;
  { SoundEffects audio; assert(audio.available()); }
  assert(FakeAudio::initializations == 0 && FakeAudio::closes == 0);
  assert(FakeAudio::ready && FakeAudio::live.empty()); // External device ownership is respected.
  FakeAudio::reset();
  std::cout << "All sound synthesis, playback, mute, and device-failure tests passed.\n";
}

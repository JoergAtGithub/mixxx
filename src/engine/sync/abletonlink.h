#pragma once

#include <ableton/Link.hpp>
#include <ableton/link/HostTimeFilter.hpp>
#include "engine/enginebuffer.h"
#include "engine/channels/enginechannel.h"
#include "engine/sync/syncable.h"
#include "engine/sync/synccontrol.h"

/// This class manages a link session. 
/// Read & update (get & set) this session for Mixxx to be a synced Link participant (bpm & phase)
///
/// Ableton Link Readme (lib/ableton-link/README.md)
/// Documentation in the header (lib/ableton-link/include/ableton/Link.hpp)
/// Ableton provides a command line tool (LinkHut) for debugging Link programs (instructions in the Readme)
/// 
/// Ableton recommends getting/setting the link session from the audio thread for maximum timing accuracy. 
/// Call the appropriate, realtime-safe functions from the audio callback to do this.

class AbletonLink : public QObject, public Syncable {
    Q_OBJECT
  public:
    AbletonLink(const QString& group, EngineSync* pEngineSync);
    ~AbletonLink() override;

    const QString& getGroup() const override {
        return m_group;
    }
    EngineChannel* getChannel() const override {
        return nullptr;
    }

    /// Notify a Syncable that their mode has changed. The Syncable must record
    /// this mode and return the latest mode in response to getMode().
    void setSyncMode(SyncMode mode) override;

    /// Notify a Syncable that it is now the only currently-playing syncable.
    void notifyUniquePlaying() override;

    /// Notify a Syncable that they should sync phase.
    void requestSync() override;

    /// Must NEVER return a mode that was not set directly via
    /// notifySyncModeChanged.
    SyncMode getSyncMode() const override;

    /// Only relevant for player Syncables.
    bool isPlaying() const override;
    bool isAudible() const override;
    bool isQuantized() const override;

    /// Gets the current speed of the syncable in bpm (bpm * rate slider), doesn't
    /// include scratch or FF/REW values.
    mixxx::Bpm getBpm() const override;

    /// Gets the beat distance as a fraction from 0 to 1
    double getBeatDistance() const override;

    /// Gets the speed of the syncable if it was playing at 1.0 rate.
    mixxx::Bpm getBaseBpm() const override;

    /// The following functions are used to tell syncables about the state of the
    /// current Sync Master.
    /// Must never result in a call to
    /// SyncableListener::notifyBeatDistanceChanged or signal loops could occur.
    void updateLeaderBeatDistance(double beatDistance) override;

    /// Must never result in a call to SyncableListener::notifyBpmChanged or
    /// signal loops could occur.
    void updateLeaderBpm(mixxx::Bpm bpm) override;

    void notifyLeaderParamSource() override;

    /// Combines the above three calls into one, since they are often set
    /// simultaneously.  Avoids redundant recalculation that would occur by
    /// using the three calls separately.
    void reinitLeaderParams(double beatDistance, mixxx::Bpm baseBpm, mixxx::Bpm bpm) override;

    /// Must never result in a call to
    /// SyncableListener::notifyInstantaneousBpmChanged or signal loops could
    /// occur.
    void updateInstantaneousBpm(mixxx::Bpm bpm) override;

    void onCallbackStart(int sampleRate, int bufferSize);
    void onCallbackEnd(int sampleRate, int bufferSize);

  private slots:
    void testPrint() {
        nonAudioPrint();
        audioSafePrint();
    }

  private:
    ableton::Link m_link;
    ableton::link::HostTimeFilter<ableton::link::platform::Clock> m_hostTimeFilter;
    const QString m_group;
    EngineSync* m_pEngineSync;
    SyncMode m_mode;
    std::chrono::microseconds m_sampleTimeAtStartCallback;
    std::chrono::microseconds m_currentLatency;
    std::chrono::microseconds m_timeAtStartCallback;
    std::chrono::microseconds m_hostTimeAtStartCallback;

    ControlPushButton* m_pLinkButton;
    std::unique_ptr<ControlObject> m_pNumLinkPeers;

    void slotControlSyncEnabled(double value);

    void readAudioBufferMicros();
    std::chrono::microseconds getHostTime() const;
    std::chrono::microseconds getHostTimeAtSpeaker(std::chrono::microseconds hostTime) const;

    double getQuantum() const {
        return 4.0;
    }

    // -----------     Test/DEBUG stuff below     ----------------

    QTimer* m_pTestTimer;
    const double beat = 0.0;

    // Link getters to call from audio thread.
    void audioSafePrint();

    /// Link getters to call from non-audio thread.
    void nonAudioPrint();

    /// Link setters to call from audio thread.
    void audioSafeSet();

    void initTestTimer(int ms, bool isRepeating);
};

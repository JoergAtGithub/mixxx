#include "engine/sync/abletonlink.h"

#include <QtDebug>
#include <cmath>

#include "control/controlobject.h"
#include "control/controlpushbutton.h"
#include "engine/sync/enginesync.h"
#include "preferences/usersettings.h"
#include "util/logger.h"

namespace {
const mixxx::Logger kLogger("AbletonLink");

} // namespace

AbletonLink::AbletonLink(const QString& group, EngineSync* pEngineSync)
        : m_link(120.), m_group(group), m_pEngineSync(pEngineSync), m_mode(SyncMode::None), m_currentLatency(0), m_hostTime(0) {

    m_pLinkButton = new ControlPushButton(ConfigKey(group, "sync_enabled"));
    m_pLinkButton->setButtonMode(ControlPushButton::TOGGLE);
    m_pLinkButton->setStates(2);

    connect(m_pLinkButton,
            &ControlObject::valueChanged,
            this,
            &AbletonLink::slotControlSyncEnabled,
            Qt::DirectConnection);

    m_pNumLinkPeers = std::make_unique<ControlObject>(ConfigKey(m_group, "num_peers"));
    m_pNumLinkPeers->setReadOnly();
    m_pNumLinkPeers->forceSet(0);

    // The callback is invoked on a Link - managed thread.
    // The callback is the only entity, which access m_pNumLinkPeers
    m_link.setNumPeersCallback([this](std::size_t numPeers) {
        m_pNumLinkPeers->forceSet(numPeers);
        });


    m_link.enable(false);
    m_link.enableStartStopSync(false);

    nonAudioPrint();
    audioSafePrint();

    initTestTimer(1000, true);
}

AbletonLink::~AbletonLink() {
    delete m_pLinkButton;
}

void AbletonLink::slotControlSyncEnabled(double value) {
    qDebug() << "AbletonLink::slotControlSyncEnabled" << value;
    if (value > 0) {
        m_link.enable(true);
        m_link.enableStartStopSync(true);
    } else {
        m_link.enable(false);
        m_link.enableStartStopSync(false);
    }
}

void AbletonLink::setSyncMode(SyncMode mode) {
    m_mode = mode;
}

void AbletonLink::notifyUniquePlaying() {
}

void AbletonLink::requestSync() {
}

SyncMode AbletonLink::getSyncMode() const {
    return m_mode;
}

bool AbletonLink::isPlaying() const {
    return false;
}
bool AbletonLink::isAudible() const {
    return false;
}
bool AbletonLink::isQuantized() const {
    return false; //TODO: Check this
}

mixxx::Bpm AbletonLink::getBpm() const {
    return getBaseBpm();
}

double AbletonLink::getBeatDistance() const {
    ableton::Link::SessionState sessionState = m_link.captureAudioSessionState();
    auto beats = sessionState.beatAtTime(getHostTimeAtSpeaker(), getQuantum());
    return std::fmod(beats, 1.);
}

mixxx::Bpm AbletonLink::getBaseBpm() const {
    ableton::Link::SessionState sessionState = m_link.captureAudioSessionState();
    mixxx::Bpm tempo(sessionState.tempo());
    return tempo;
}

void AbletonLink::readAudioBufferMicros() {
    if ((m_pEngineSync != nullptr) &&
            (m_pEngineSync->getLeaderChannel() != nullptr) &&
            (m_pEngineSync->getLeaderChannel()->getEngineBuffer() != nullptr)) {
        // Only set a new buffer size if we can determine it, otherwise we assume that it's not changed
        m_currentLatency = std::chrono::microseconds(m_pEngineSync->getLeaderChannel()->getEngineBuffer()->getAudioBufferMicros());
    };
}

// Approximate the system time when the first sample in the current audio buffer will hit the speakers
std::chrono::microseconds AbletonLink::getHostTimeAtSpeaker() const {

    return m_hostTime - m_currentLatency;
    // return m_link.clock().micros() + m_currentLatency;
}

void AbletonLink::updateLeaderBeatDistance(double beatDistance) {
    ableton::Link::SessionState sessionState = m_link.captureAudioSessionState();

    auto currentBeat = sessionState.beatAtTime(getHostTimeAtSpeaker(), getQuantum());
    auto newBeat = currentBeat - std::fmod(currentBeat, 1.0) + beatDistance;
    sessionState.requestBeatAtTime(newBeat, getHostTimeAtSpeaker(), getQuantum());

    m_link.commitAudioSessionState(sessionState);
}

void AbletonLink::updateLeaderBpm(mixxx::Bpm bpm) {
    ableton::Link::SessionState sessionState = m_link.captureAudioSessionState();
    sessionState.setTempo(bpm.value(), getHostTimeAtSpeaker());
    m_link.commitAudioSessionState(sessionState);
}

void AbletonLink::notifyLeaderParamSource() {
    //TODO: Not implemented yet
}

void AbletonLink::reinitLeaderParams(double beatDistance, mixxx::Bpm baseBpm, mixxx::Bpm bpm) {
    Q_UNUSED(baseBpm)
    updateLeaderBeatDistance(beatDistance);
    updateLeaderBpm(bpm);
}

void AbletonLink::updateInstantaneousBpm(mixxx::Bpm bpm) {
    updateLeaderBpm(bpm);
}

void AbletonLink::onCallbackStart(int sampleRate, int bufferSize) {
    updateHostTime(m_sampleTime);
    readAudioBufferMicros();

    //m_pNumLinkPeers->forceSet(m_link.numPeers());

}

void AbletonLink::onCallbackEnd(int sampleRate, int bufferSize) {
    Q_UNUSED(sampleRate)
    Q_UNUSED(bufferSize)
}


// **************************
// Debug ouput functions only
// **************************

void AbletonLink::audioSafePrint() {
    qDebug() << "isEnabled()" << m_link.isEnabled();
    qDebug() << "numPeers()" << m_link.numPeers();

    ableton::Link::SessionState sessionState = m_link.captureAudioSessionState();

    qDebug() << "sessionState.tempo()" << sessionState.tempo();
    qDebug() << "sessionState.beatAtTime()" << sessionState.beatAtTime(m_link.clock().micros(), getQuantum());
    qDebug() << "sessionState.phaseAtTime()" << sessionState.phaseAtTime(m_link.clock().micros(), getQuantum());
    qDebug() << "sessionState.timeAtBeat(0)" << sessionState.timeAtBeat(0.0, getQuantum()).count();
    qDebug() << "sessionState.isPlaying()" << sessionState.isPlaying();
    qDebug() << "sessionState.timeForIsPlaying()" << sessionState.timeForIsPlaying().count();

    // Est. Delay (micro-seconds) between onCallbackStart() and buffer's first audio sample reaching speakers
    qDebug() << "Est. Delay (us)" << (getHostTimeAtSpeaker() - m_link.clock().micros()).count();
}

void AbletonLink::nonAudioPrint() {
    qDebug() << "isStartStopSyncEnabled()" << m_link.isStartStopSyncEnabled();
}

void AbletonLink::audioSafeSet() {
    ableton::Link::SessionState sessionState = m_link.captureAudioSessionState();

    sessionState.setTempo(120, m_link.clock().micros());
    sessionState.requestBeatAtTime(beat, m_link.clock().micros(), getQuantum());
    sessionState.setIsPlaying(true, m_link.clock().micros());

    // convenience functions
    sessionState.requestBeatAtStartPlayingTime(beat, getQuantum());
    sessionState.setIsPlayingAndRequestBeatAtTime(true, m_link.clock().micros(), beat, getQuantum());

    m_link.commitAudioSessionState(sessionState);
}

void AbletonLink::initTestTimer(int ms, bool isRepeating) {
    m_pTestTimer = new QTimer(this);
    connect(m_pTestTimer, &QTimer::timeout, this, QOverload<>::of(&AbletonLink::testPrint));
    m_pTestTimer->setSingleShot(!isRepeating);
    m_pTestTimer->start(ms);
}
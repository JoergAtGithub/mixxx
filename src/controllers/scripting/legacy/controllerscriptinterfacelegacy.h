#pragma once

#include <QJSValue>
#include <QObject>

#include "controllers/softtakeover.h"
#include "util/alphabetafilter.h"
#include "util/runtimeloggingcategory.h"

class ControllerScriptEngineLegacy;
class ControlObjectScript;
class ScriptConnection;
class ConfigKey;

/// ControllerScriptInterfaceLegacy is the legacy API for controller scripts to interact
/// with Mixxx. It is inserted into the JS environment as the "engine" object.
class ControllerScriptInterfaceLegacy : public QObject {
    Q_OBJECT
  public:
    enum class WellKnownCharsets {
        Latin1,
        ISO_8859_1,
        Latin9,
        ISO_8859_15,
        UCS2,
        ISO_10646_UCS_2,
        UTF_8,
        UTF_16,
        UTF_16BE,
        UTF_16LE,
        UTF_32,
        UTF_32BE,
        UTF_32LE,
        /* Platform endianess is not supported Qt <6.4
        * and is not relevant for controller scripting
        * as the host computer platform is not the target
        UTF16_PlatformEndian,
        UTF16_OppositeEndian,
        UTF32_PlatformEndian,
        UTF32_OppositeEndian,
        UTF_16BE_Version_1,
        UTF_16LE_Version_1,
        UTF_16_Version_1,
        UTF_16_Version_2,*/
        UTF_7,
        // IMAP_Mailbox_Name,
        SCSU,
        BOCU_1,
        CESU_8,
        US_ASCII,
        GB18030,
        ISO_8859_2,
        ISO_8859_3,
        ISO_8859_4,
        ISO_8859_5,
        ISO_8859_6,
        ISO_8859_7,
        // IBM_813_P100_1995,
        ISO_8859_8,
        // IBM_916_P100_1995,
        ISO_8859_9,
        ISO_8859_10,
        ISO_8859_11_2001,
        ISO_8859_13,
        ISO_8859_14,
        IBM_942_P12A_1999,
        Shift_JIS,
        IBM_943_P130_1999,
        IBM_33722_P12A_P12A_2009_U2,
        IBM_33722_P120_1999,
        IBM_954_P101_2007,
        EUC_JP,
        IBM_1373_P100_2002,
        Big5,
        IBM_950_P110_1999,
        Big5_HKSCS,
        IBM_5471_P100_2006,
        IBM_1386_P100_2001,
        GBK,
        GB2312,
        // GB_2312_80, => GB_2312-80 results in an infinite loop in ICU and
        // stalls the whole QJSEngine theads
        EUC_TW_2014,
        IBM_964_P110_1999,
        IBM_949_P110_1999,
        IBM_949_P11A_1999,
        EUC_KR,
        // IBM_971_P100_1995, => ibm-971_P100-1995 results in an infinite loop
        // in ICU and stalls the whole QJSEngine theads
        CP1363,
        IBM_1363_P110_1997,
        KSC_5601,
        Windows_874_2000,
        TIS_620,
        IBM_1162_P100_1999,
        IBM437,
        IBM_720_P100_1997,
        IBM_737_P100_1997,
        IBM775,
        IBM850,
        CP851,
        IBM852,
        IBM855,
        IBM_856_P100_1995,
        IBM857,
        IBM00858,
        IBM860,
        IBM861,
        IBM862,
        IBM863,
        IBM864,
        IBM865,
        IBM866,
        IBM_867_P100_1998,
        IBM868,
        IBM869,
        KOI8_R,
        IBM_901_P100_1999,
        IBM_902_P100_1999,
        IBM_922_P100_1999,
        KOI8_U,
        IBM_4909_P100_1999,
        Windows_1250,
        Windows_1251,
        Windows_1252,
        Windows_1253,
        Windows_1254,
        Windows_1255,
        Windows_1256,
        Windows_1257,
        Windows_1258,
        IBM_1250_P100_1995,
        IBM_1251_P100_1995,
        IBM_1252_P100_2000,
        IBM_1253_P100_1995,
        IBM_1254_P100_1995,
        IBM_1255_P100_1995,
        IBM_5351_P100_1998,
        IBM_1256_P110_1997,
        IBM_5352_P100_1998,
        IBM_1257_P100_1995,
        IBM_5353_P100_1998,
        IBM_1258_P100_1997,
        Macintosh,
        X_Mac_Greek,
        X_Mac_Cyrillic,
        X_Mac_CentralEuroRoman,
        X_Mac_Turkish,
        HP_Roman8,
        Adobe_Standard_Encoding,
        IBM_1006_P100_1995,
        IBM_1098_P100_1995,
        IBM_1124_P100_1996,
        IBM_1125_P100_1997,
        IBM_1129_P100_1997,
        IBM_1131_P100_1997,
        IBM_1133_P100_1997,
        GSM_03_38_2009,
        ISO_2022_JP,
        ISO_2022_JP_1,
        ISO_2022_JP_2,
        ISO_2022_Locale_JA_Version_3,
        ISO_2022_Locale_JA_Version_4,
        ISO_2022_KR,
        ISO_2022_Locale_KO_Version_1,
        ISO_2022_CN,
        ISO_2022_CN_EXT,
        ISO_2022_Locale_ZH_Version_2,
        HZ_GB_2312,
        X11_Compound_Text,
        ISCII_Version_0,
        ISCII_Version_1,
        ISCII_Version_2,
        ISCII_Version_3,
        ISCII_Version_4,
        ISCII_Version_5,
        ISCII_Version_6,
        ISCII_Version_7,
        ISCII_Version_8,
        LMBCS_1,
        IBM037,
        IBM273,
        IBM277,
        IBM278,
        IBM280,
        IBM284,
        IBM285,
        IBM290,
        IBM297,
        IBM420,
        IBM424,
        IBM500,
        IBM_803_P100_1999,
        IBM_Thai,
        IBM870,
        IBM871,
        IBM_875_P100_1995,
        IBM918,
        IBM_930_P120_1999,
        IBM_933_P110_1995,
        IBM_935_P110_1999,
        IBM_937_P110_1999,
        IBM_939_P120_1999,
        IBM_1025_P100_1995,
        IBM1026,
        IBM1047,
        IBM_1097_P100_1995,
        IBM_1112_P100_1995,
        IBM_1122_P100_1999,
        IBM_1123_P100_1995,
        IBM_1130_P100_1997,
        IBM_1132_P100_1998,
        IBM_1137_P100_1999,
        IBM_4517_P100_2005,
        IBM01140,
        IBM01141,
        IBM01142,
        IBM01143,
        IBM01144,
        IBM01145,
        IBM01146,
        IBM01147,
        IBM01148,
        IBM01149,
        IBM_1153_P100_1999,
        IBM_1154_P100_1999,
        IBM_1155_P100_1999,
        IBM_1156_P100_1999,
        IBM_1157_P100_1999,
        IBM_1158_P100_1999,
        IBM_1160_P100_1999,
        IBM_1164_P100_1999,
        IBM_1364_P110_2007,
        IBM_1371_P100_1999,
        IBM_1388_P103_2001,
        IBM_1390_P110_2003,
        IBM_1399_P110_2003,
        IBM_5123_P100_1999,
        IBM_8482_P100_1999,
        // IBM_16684_P110_2003, => ibm-16684_P110-2003 results in an infinite
        // loop in ICU and stalls the whole QJSEngine theads
        IBM_4899_P100_1998,
        IBM_4971_P100_1999,
        IBM_9067_X100_2005,
        IBM_12712_P100_1998,
        IBM_16804_X110_1999,
        IBM_37_P100_1995_SwapLFNL,
        IBM_1047_P100_1995_SwapLFNL,
        IBM_1140_P100_1997_SwapLFNL,
        IBM_1141_P100_1997_SwapLFNL,
        IBM_1142_P100_1997_SwapLFNL,
        IBM_1143_P100_1997_SwapLFNL,
        IBM_1144_P100_1997_SwapLFNL,
        IBM_1145_P100_1997_SwapLFNL,
        IBM_1146_P100_1997_SwapLFNL,
        IBM_1147_P100_1997_SwapLFNL,
        IBM_1148_P100_1997_SwapLFNL,
        IBM_1149_P100_1997_SwapLFNL,
        IBM_1153_P100_1999_SwapLFNL,
        IBM_12712_P100_1998_SwapLFNL,
        IBM_16804_X110_1999_SwapLFNL,
        EBCDIC_XML_US
    };
    Q_ENUM(WellKnownCharsets)

    ControllerScriptInterfaceLegacy(ControllerScriptEngineLegacy* m_pEngine,
            const RuntimeLoggingCategory& logger);

    virtual ~ControllerScriptInterfaceLegacy();

    Q_INVOKABLE QJSValue getSetting(const QString& name);
    Q_INVOKABLE double getValue(const QString& group, const QString& name);
    Q_INVOKABLE void setValue(const QString& group, const QString& name, double newValue);
    Q_INVOKABLE double getParameter(const QString& group, const QString& name);
    Q_INVOKABLE void setParameter(const QString& group, const QString& name, double newValue);
    Q_INVOKABLE double getParameterForValue(
            const QString& group, const QString& name, double value);
    Q_INVOKABLE void reset(const QString& group, const QString& name);
    Q_INVOKABLE double getDefaultValue(const QString& group, const QString& name);
    Q_INVOKABLE double getDefaultParameter(const QString& group, const QString& name);
    Q_INVOKABLE QJSValue makeConnection(const QString& group,
            const QString& name,
            const QJSValue& callback);
    Q_INVOKABLE QJSValue makeUnbufferedConnection(const QString& group,
            const QString& name,
            const QJSValue& callback);
    // DEPRECATED: Use makeConnection instead.
    Q_INVOKABLE QJSValue connectControl(const QString& group,
            const QString& name,
            const QJSValue& passedCallback,
            bool disconnect = false);
    // Called indirectly by the objects returned by connectControl
    Q_INVOKABLE void trigger(const QString& group, const QString& name);
    // DEPRECATED: Use console.log instead.
    Q_INVOKABLE void log(const QString& message);
    Q_INVOKABLE int beginTimer(int interval, QJSValue scriptCode, bool oneShot = false);
    Q_INVOKABLE void stopTimer(int timerId);
    Q_INVOKABLE void scratchEnable(int deck,
            int intervalsPerRev,
            double rpm,
            double alpha,
            double beta,
            bool ramp = true);
    Q_INVOKABLE void scratchTick(int deck, int interval);
    Q_INVOKABLE void scratchDisable(int deck, bool ramp = true);
    Q_INVOKABLE bool isScratching(int deck);
    Q_INVOKABLE void softTakeover(const QString& group, const QString& name, bool set);
    Q_INVOKABLE void softTakeoverIgnoreNextValue(const QString& group, const QString& name);
    Q_INVOKABLE bool softTakeoverWillIgnore(
            const QString& group, const QString& name, double parameter);
    Q_INVOKABLE void brake(const int deck,
            bool activate,
            double factor = 1.0,
            const double rate = 1.0);
    Q_INVOKABLE void spinback(const int deck,
            bool activate,
            double factor = 1.8,
            const double rate = -10.0);
    Q_INVOKABLE void softStart(const int deck, bool activate, double factor = 1.0);

    Q_INVOKABLE QByteArray convertCharset(
            const ControllerScriptInterfaceLegacy::WellKnownCharsets
                    targetCharset,
            const QString& value);

    bool removeScriptConnection(const ScriptConnection& conn);
    /// Execute a ScriptConnection's JS callback
    void triggerScriptConnection(const ScriptConnection& conn);

    /// Handler for timers that scripts set.
    virtual void timerEvent(QTimerEvent* event);

  private:
    QByteArray convertCharsetInternal(const QString& targetCharset, const QString& value);
    QJSValue makeConnectionInternal(const QString& group,
            const QString& name,
            const QJSValue& callback,
            bool skipSuperseded = false);
    QHash<ConfigKey, ControlObjectScript*> m_controlCache;
    ControlObjectScript* getControlObjectScript(const QString& group, const QString& name);

    SoftTakeoverCtrl m_st;

    struct TimerInfo {
        QJSValue callback;
        bool oneShot;
    };
    QHash<int, TimerInfo> m_timers;

    QVarLengthArray<int> m_intervalAccumulator;
    QVarLengthArray<mixxx::Duration> m_lastMovement;
    QVarLengthArray<double> m_dx, m_rampTo, m_rampFactor;
    QVarLengthArray<bool> m_ramp, m_brakeActive, m_spinbackActive, m_softStartActive;
    QVarLengthArray<AlphaBetaFilter*> m_scratchFilters;
    QHash<int, int> m_scratchTimers;
    /// Applies the accumulated movement to the track speed
    void scratchProcess(int timerId);
    void stopScratchTimer(int timerId);
    bool isDeckPlaying(const QString& group);
    void stopDeck(const QString& group);
    bool isTrackLoaded(const QString& group);
    double getDeckRate(const QString& group);

    ControllerScriptEngineLegacy* m_pScriptEngineLegacy;
    const RuntimeLoggingCategory m_logger;
};

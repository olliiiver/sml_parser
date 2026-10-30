#ifndef SML_H
#define SML_H

/*!
 * \file Parse and interpret an SML (Smart Message Language) message.
 *
 * Use \ref smlState (or \ref SmlParser::smlState) to parse the message byte by byte.
 * This will cache the parsed data in the memory provided by the global instance \ref ::SmlParser1.
 * To access values, check whether the OBIS code you are interested in has just
 * been parsed via \ref smlOBISCheck (or \ref SmlParser::smlOBISCheck) and if so, parse that value
 * using \ref smlOBISByUnit (or \ref SmlParser::smlOBISByUnit) to get the data in full precision or
 * one of the convenience functions like \ref smlOBISW (or \ref SmlParser::smlOBISW) to get
 * the data directly as double in the specified unit.
 */

#include <stdbool.h>

typedef enum : unsigned char {
  SML_START,
  SML_END,
  SML_VERSION,
  SML_NEXT,
  SML_LISTSTART,
  SML_LISTEND,
  SML_LISTEXTENDED,
  SML_DATA,
  SML_HDATA,
  SML_DATAEND,
  SML_BLOCKSTART,
  SML_BLOCKEND,
  SML_CHECKSUM,
  SML_CHECKSUM_ERROR, /* calculated checksum does not match */
  SML_UNEXPECTED,     /* unexpected byte received */
  SML_FINAL,          /* final state, checksum OK */
  SML_DATA_SIGNED_INT,
  SML_DATA_UNSIGNED_INT,
  SML_DATA_OCTET_STRING,
} sml_states_t;

typedef enum {
  SML_YEAR = 1,
  SML_MONTH = 2,
  SML_WEEK = 3,
  SML_DAY = 4,
  SML_HOUR = 5,
  SML_MIN = 6,
  SML_SECOND = 7,
  SML_DEGREE = 8,
  SML_DEGREE_CELSIUS = 9,
  SML_CURRENCY = 10,
  SML_METRE = 11,
  SML_METRE_PER_SECOND = 12,
  SML_CUBIC_METRE = 13,
  SML_CUBIC_METRE_CORRECTED = 14,
  SML_CUBIC_METRE_PER_HOUR = 15,
  SML_CUBIC_METRE_PER_HOUR_CORRECTED = 16,
  SML_CUBIC_METRE_PER_DAY = 17,
  SML_CUBIC_METRE_PER_DAY_CORRECTED = 18,
  SML_LITRE = 19,
  SML_KILOGRAM = 20,
  SML_NEWTON = 21,
  SML_NEWTONMETER = 22,
  SML_PASCAL = 23,
  SML_BAR = 24,
  SML_JOULE = 25,
  SML_JOULE_PER_HOUR = 26,
  SML_WATT = 27,
  SML_VOLT_AMPERE = 28,
  SML_VAR = 29,
  SML_WATT_HOUR = 30,
  SML_VOLT_AMPERE_HOUR = 31,
  SML_VAR_HOUR = 32,
  SML_AMPERE = 33,
  SML_COULOMB = 34,
  SML_VOLT = 35,
  SML_VOLT_PER_METRE = 36,
  SML_FARAD = 37,
  SML_OHM = 38,
  SML_OHM_METRE = 39,
  SML_WEBER = 40,
  SML_TESLA = 41,
  SML_AMPERE_PER_METRE = 42,
  SML_HENRY = 43,
  SML_HERTZ = 44,
  SML_ACTIVE_ENERGY_METER_CONSTANT_OR_PULSE_VALUE = 45,
  SML_REACTIVE_ENERGY_METER_CONSTANT_OR_PULSE_VALUE = 46,
  SML_APPARENT_ENERGY_METER_CONSTANT_OR_PULSE_VALUE = 47,
  SML_VOLT_SQUARED_HOURS = 48,
  SML_AMPERE_SQUARED_HOURS = 49,
  SML_KILOGRAM_PER_SECOND = 50,
  SML_KELVIN = 52,
  SML_VOLT_SQUARED_HOUR_METER_CONSTANT_OR_PULSE_VALUE = 53,
  SML_AMPERE_SQUARED_HOUR_METER_CONSTANT_OR_PULSE_VALUE = 54,
  SML_METER_CONSTANT_OR_PULSE_VALUE = 55,
  SML_PERCENTAGE = 56,
  SML_AMPERE_HOUR = 57,
  SML_ENERGY_PER_VOLUME = 60,
  SML_CALORIFIC_VALUE = 61,
  SML_MOLE_PERCENT = 62,
  SML_MASS_DENSITY = 63,
  SML_PASCAL_SECOND = 64,
  SML_RESERVED = 253,
  SML_OTHER_UNIT = 254,
  SML_COUNT = 255
} sml_units_t;

/*! A single entry of a list of values (SML_ListEntry), as returned by
 * \ref smlOBISEntry. It allows to read all values of a meter without knowing
 * their OBIS codes in advance.
 */
typedef struct {
  unsigned char obis[6]; /* OBIS code `{A, B, C, D, E, F}` */
  unsigned char unit;    /* sml_units_t, 0 if the meter did not send a unit */
  signed char scaler;    /* the value is `value x 10^scaler` */
  sml_states_t type;     /* SML_DATA_SIGNED_INT, SML_DATA_UNSIGNED_INT,
                            SML_DATA_OCTET_STRING or SML_DATA (other) */
  long long int value;   /* numeric value, only set for integer types */
  const unsigned char *bytes; /* raw value, points into the parser buffer and
                                 is only valid until the next byte is parsed */
  unsigned char length;       /* number of raw value bytes */
} sml_obis_entry_t;

/*! Parse a single byte of an SML message and return the status after parsing
 * that. */
sml_states_t smlState(unsigned char byte);

/* ! Return whether the 6-character OBIS identifier (binary encoded) matches to
 * the last received message.
 * If it does, use one of the specific functions to retrieve the value.
 *
 * Calling this function makes sense after a list was received and parsed, i.e.
 * after the state was \ref SML_LISTEND .
 *
 * OBIS (Object Identification System) identifies the kind of message that was
 * received in the format `A-B:C.D.E*F`, the encoding in \p obis is
 * an array `{A, B, C, D, E, F}`.
 * For example `{0x01, 0x00, 0x01, 0x08, 0x01, 0xff}` encodes `1-0:1.8.1*255`,
 which is the electric meter reading.
 * If the message matches this particular OBIS identifier, we know that a meter
 reading in Wh (Watt hours) was received and we can then read it using \ref
 smlOBISWh.
 */
bool smlOBISCheck(const unsigned char *obis);

/*! Fill \p entry with OBIS code, unit, scaler and value of the last received
 * list, if that list is a complete entry of a list of values.
 *
 * Call this after the state was \ref SML_LISTEND . In contrast to
 * \ref smlOBISCheck it does not need to know the OBIS code in advance, e.g. to
 * print all values a meter sends.
 *
 * \return true if \p entry was filled, false if the last list is no entry.
 */
bool smlOBISEntry(sml_obis_entry_t &entry);

/*! Format the binary OBIS code \p obis as `A-B:C.D.E*F` into \p str
 * (e.g. `1-0:1.8.0*255`), at most \p maxSize bytes including the terminator.
 *
 * \return the number of characters that would have been written (snprintf).
 */
int smlOBISToString(const unsigned char *obis, char *str, int maxSize);

/*! Parse an OBIS code like `1-0:1.8.0*255` from \p str into the 6 byte binary
 * form \p obis, e.g. to build handlers from a configuration at runtime. If `*F`
 * is omitted (`1-0:1.8.0`), F is set to 255.
 *
 * \return true on success, false if \p str is no valid OBIS code.
 */
bool smlOBISFromString(const char *str, unsigned char *obis);

/*! Copy the first \p maxSize bytes of the name/identifier of the manufacturer
 * into buffer \p str. Use this after reading OBIS code `{0x81, 0x81, 0xc7,
 * 0x82, 0x03, 0xff}` i.e. 129-129:199.130.3*255.
 */
void smlOBISManufacturer(unsigned char *str, int maxSize);

/*! Copies the unique id (server id) of the smart meter device into the
 * provided buffer \p bytes, at most \p maxSize bytes.
 * Use this after reading OBIS code `{0x01, 0x00, 0x00, 0x00, 0x09, 0xff}`
 * i.e. 1-0:0.0.9*255, or after OBIS code
 * `{0x01, 0x00, 0x60, 0x01, 0x00, 0xff}` i.e. 1-0:96.1.0*255 (device id).
 *
 * \return Returns the number of bytes copied to the buffer.
 */
int smlOBISServerId(unsigned char* bytes, int maxSize);

/*! Copy the value last read into \p val and the read scaler into \p scaler and
 * ensure that it has unit \p unit . If the unit was assumed wrongly, set \p val
 * to -1.
 * The final value is `val x 10^scaler`.
 *
 * There are 'convenience' functions that wrap this function to return the
 * actual value as double, see below.
 *
 * \return nothing, but set \p val to -1 in case of an error.
 */
void smlOBISByUnit(long long int &val, signed char &scaler, sml_units_t unit);

// Be aware that double on Arduino UNO is just 32 bit

/*! Convenience function to get a reading in Watt hours.*/
void smlOBISWh(double &wh);
/*! Convenience function to get a reading in Watts.*/
void smlOBISW(double &w);
/*! Convenience function to get a reading in Volts.*/
void smlOBISVolt(double &v);
/*! Convenience function to get a reading in Amperes.*/
void smlOBISAmpere(double &a);
/*! Convenience function to get a reading in Hertz.*/
void smlOBISHertz(double &h);
/*! Convenience function to get a reading in Degrees.*/
void smlOBISDegree(double &d);

/*! Provides parsing functionality for SML messages */
class SmlParser
{
  public:
    SmlParser();

    /*! Parse a single byte of an SML message and return the status after parsing
     * that. */
    sml_states_t smlState(unsigned char byte);

    /* ! Return whether the 6-character OBIS identifier (binary encoded) matches to
     * the last received message.
     * If it does, use one of the specific functions to retrieve the value.
     *
     * Calling this function makes sense after a list was received and parsed, i.e.
     * after the state was \ref SML_LISTEND .
     *
     * OBIS (Object Identification System) identifies the kind of message that was
     * received in the format `A-B:C.D.E*F`, the encoding in \p obis is
     * an array `{A, B, C, D, E, F}`.
     * For example `{0x01, 0x00, 0x01, 0x08, 0x01, 0xff}` encodes `1-0:1.8.1*255`,
     which is the electric meter reading.
     * If the message matches this particular OBIS identifier, we know that a meter
     reading in Wh (Watt hours) was received and we can then read it using \ref
     smlOBISWh.
     */
    bool smlOBISCheck(const unsigned char *obis);

    /*! Fill \p entry with OBIS code, unit, scaler and value of the last received
     * list, if that list is a complete entry of a list of values.
     *
     * Call this after the state was \ref SML_LISTEND . In contrast to
     * \ref smlOBISCheck it does not need to know the OBIS code in advance, e.g. to
     * print all values a meter sends.
     *
     * \return true if \p entry was filled, false if the last list is no entry.
     */
    bool smlOBISEntry(sml_obis_entry_t &entry);

    /*! Copy the first \p maxSize bytes of the name/identifier of the manufacturer
     * into buffer \p str. Use this after reading OBIS code `{0x81, 0x81, 0xc7,
     * 0x82, 0x03, 0xff}` i.e. 129-129:199.130.3*255.
     */
    void smlOBISManufacturer(unsigned char *str, int maxSize);
	
    /*! Copies the unique id (server id) of the smart meter device into the
     * provided buffer \p bytes, at most \p maxSize bytes.
     * Use this after reading OBIS code `{0x01, 0x00, 0x00, 0x00, 0x09, 0xff}`
     * i.e. 1-0:0.0.9*255, or after OBIS code
     * `{0x01, 0x00, 0x60, 0x01, 0x00, 0xff}` i.e. 1-0:96.1.0*255 (device id).
     *
     * \return Returns the number of bytes copied to the buffer.
     */
    int smlOBISServerId(unsigned char* bytes, int maxSize);

    /*! Copy the value last read into \p val and the read scaler into \p scaler and
     * ensure that it has unit \p unit . If the unit was assumed wrongly, set \p val
     * to -1.
     * The final value is `val x 10^scaler`.
     *
     * There are 'convenience' functions that wrap this function to return the
     * actual value as double, see below.
     *
     * \return nothing, but set \p val to -1 in case of an error.
     */
    void smlOBISByUnit(long long int &val, signed char &scaler, sml_units_t unit);

    // Be aware that double on Arduino UNO is just 32 bit

    /*! Convenience function to get a reading in Watt hours.*/
    void smlOBISWh(double &wh);
    /*! Convenience function to get a reading in Watts.*/
    void smlOBISW(double &w);
    /*! Convenience function to get a reading in Volts.*/
    void smlOBISVolt(double &v);
    /*! Convenience function to get a reading in Amperes.*/
    void smlOBISAmpere(double &a);
    /*! Convenience function to get a reading in Hertz.*/
    void smlOBISHertz(double &h);
    /*! Convenience function to get a reading in Degrees.*/
    void smlOBISDegree(double &d);

    /*! Resets the state of the instance.*/
	void reset();
	
  private:
    void crc16(unsigned char byte);
    void setState(sml_states_t state, int byteLen);
    void pushListBuffer(unsigned char byte);
    void smlNewList(unsigned char size);
    void checkMagicByte(unsigned char byte);
    void reduceList();

	static const int MAX_LIST_SIZE = 80;
	static const int MAX_TREE_SIZE = 10;

    sml_states_t currentState = SML_START;
    unsigned char nodes[MAX_TREE_SIZE]; /* list sizes go up to 255 */
    unsigned char currentLevel = 0;
    unsigned short crc = 0xFFFF;
    signed char sc = 0;
    unsigned short crcMine = 0xFFFF;
    unsigned short crcReceived = 0x0000;
    unsigned char len = 4;
    unsigned char listBuffer[MAX_LIST_SIZE]; /* keeps a list
	                                            as length + state + data  */
    unsigned char listPos = 0;
#ifdef SML_DEBUG
    char logBuff[200];
#endif
};

/*! Provides the parsing state and cached values for the
 * SML message as passed in through \ref smlState or \ref SmlParser::smlState.
 * In case it is necessary to parse the SML messages of two (or more)
 * smart meter devices simultaneously, additional instances should be
 * provided in the main program and used for the SML messages of the
 * other smart meter devices.
 */
extern SmlParser SmlParser1;

#endif

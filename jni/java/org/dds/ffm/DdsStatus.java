/*
   DDS, a bridge double dummy solver.

   DDS status codes returned by the solver entry points. These mirror the
   RETURN_* macros in library/src/api/dll.h one-for-one (same names and values)
   so Java callers can compare a return code against a named constant instead of
   a bare magic number. Keep this in sync with dll.h.

   See LICENSE and README.
*/

package org.dds.ffm;

/**
 * Status codes returned by {@link Dds#solveBoard}, {@link Dds#calcDdTable}, and
 * {@link Dds#calcPar} (and the flat C API). Values match the {@code RETURN_*}
 * macros in {@code library/src/api/dll.h}. Not instantiable; use the constants
 * and {@link #name(int)}.
 */
public final class DdsStatus {

    private DdsStatus() {
    }

    /** Success. */
    public static final int RETURN_NO_FAULT = 1;
    /** General error. */
    public static final int RETURN_UNKNOWN_FAULT = -1;
    /** Zero cards. */
    public static final int RETURN_ZERO_CARDS = -2;
    /** Target exceeds number of tricks. */
    public static final int RETURN_TARGET_TOO_HIGH = -3;
    /** Cards duplicated. */
    public static final int RETURN_DUPLICATE_CARDS = -4;
    /** Target is less than -1. */
    public static final int RETURN_TARGET_WRONG_LO = -5;
    /** Target is higher than 13. */
    public static final int RETURN_TARGET_WRONG_HI = -7;
    /** Solutions parameter is less than 1. */
    public static final int RETURN_SOLNS_WRONG_LO = -8;
    /** Solutions parameter is higher than 3. */
    public static final int RETURN_SOLNS_WRONG_HI = -9;
    /** Too many cards. */
    public static final int RETURN_TOO_MANY_CARDS = -10;
    /** A card is neither a suit nor a rank. */
    public static final int RETURN_SUIT_OR_RANK = -12;
    /** Played card also remains in a hand. */
    public static final int RETURN_PLAYED_CARD = -13;
    /** Wrong number of remaining cards in a hand. */
    public static final int RETURN_CARD_COUNT = -14;
    /** Thread index is not 0 .. maximum. */
    public static final int RETURN_THREAD_INDEX = -15;
    /** Mode parameter is less than 0. */
    public static final int RETURN_MODE_WRONG_LO = -16;
    /** Mode parameter is higher than 2. */
    public static final int RETURN_MODE_WRONG_HI = -17;
    /** Trump is not in 0 .. 4. */
    public static final int RETURN_TRUMP_WRONG = -18;
    /** First is not in 0 .. 2. */
    public static final int RETURN_FIRST_WRONG = -19;
    /** AnalysePlay input error. */
    public static final int RETURN_PLAY_FAULT = -98;
    /** PBN string error. */
    public static final int RETURN_PBN_FAULT = -99;
    /** Too many boards requested. */
    public static final int RETURN_TOO_MANY_BOARDS = -101;
    /** Could not create threads. */
    public static final int RETURN_THREAD_CREATE = -102;
    /** Something failed waiting for a thread to end. */
    public static final int RETURN_THREAD_WAIT = -103;
    /** Multi-threading system not present. */
    public static final int RETURN_THREAD_MISSING = -104;
    /** Denomination filter vector has no entries. */
    public static final int RETURN_NO_SUIT = -201;
    /** Too many DD tables requested. */
    public static final int RETURN_TOO_MANY_TABLES = -202;
    /** Chunk size is less than 1. */
    public static final int RETURN_CHUNK_SIZE = -301;

    /**
     * Symbolic name of a status code (e.g. {@code "RETURN_TRUMP_WRONG"}), or
     * {@code "RETURN(<code>)"} for an unrecognised value. Handy for assertion
     * and log messages.
     */
    public static String name(int code) {
        switch (code) {
            case RETURN_NO_FAULT: return "RETURN_NO_FAULT";
            case RETURN_UNKNOWN_FAULT: return "RETURN_UNKNOWN_FAULT";
            case RETURN_ZERO_CARDS: return "RETURN_ZERO_CARDS";
            case RETURN_TARGET_TOO_HIGH: return "RETURN_TARGET_TOO_HIGH";
            case RETURN_DUPLICATE_CARDS: return "RETURN_DUPLICATE_CARDS";
            case RETURN_TARGET_WRONG_LO: return "RETURN_TARGET_WRONG_LO";
            case RETURN_TARGET_WRONG_HI: return "RETURN_TARGET_WRONG_HI";
            case RETURN_SOLNS_WRONG_LO: return "RETURN_SOLNS_WRONG_LO";
            case RETURN_SOLNS_WRONG_HI: return "RETURN_SOLNS_WRONG_HI";
            case RETURN_TOO_MANY_CARDS: return "RETURN_TOO_MANY_CARDS";
            case RETURN_SUIT_OR_RANK: return "RETURN_SUIT_OR_RANK";
            case RETURN_PLAYED_CARD: return "RETURN_PLAYED_CARD";
            case RETURN_CARD_COUNT: return "RETURN_CARD_COUNT";
            case RETURN_THREAD_INDEX: return "RETURN_THREAD_INDEX";
            case RETURN_MODE_WRONG_LO: return "RETURN_MODE_WRONG_LO";
            case RETURN_MODE_WRONG_HI: return "RETURN_MODE_WRONG_HI";
            case RETURN_TRUMP_WRONG: return "RETURN_TRUMP_WRONG";
            case RETURN_FIRST_WRONG: return "RETURN_FIRST_WRONG";
            case RETURN_PLAY_FAULT: return "RETURN_PLAY_FAULT";
            case RETURN_PBN_FAULT: return "RETURN_PBN_FAULT";
            case RETURN_TOO_MANY_BOARDS: return "RETURN_TOO_MANY_BOARDS";
            case RETURN_THREAD_CREATE: return "RETURN_THREAD_CREATE";
            case RETURN_THREAD_WAIT: return "RETURN_THREAD_WAIT";
            case RETURN_THREAD_MISSING: return "RETURN_THREAD_MISSING";
            case RETURN_NO_SUIT: return "RETURN_NO_SUIT";
            case RETURN_TOO_MANY_TABLES: return "RETURN_TOO_MANY_TABLES";
            case RETURN_CHUNK_SIZE: return "RETURN_CHUNK_SIZE";
            default: return "RETURN(" + code + ")";
        }
    }
}

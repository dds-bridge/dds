/*
   DDS, a bridge double dummy solver.

   End-to-end FFM smoke test: load the native shared library, query GetDDSInfo,
   then create a solver context and solve a known deal through the dds_c_* shim,
   asserting the double dummy result. Mirrors python/tests/test_import.py /
   python_interface_smoke_test.

   Run as a plain main (use_testrunner = False): success exits 0, any failure
   throws and fails the test.

   See LICENSE and README.
*/

package org.dds.ffm;

import static java.lang.foreign.ValueLayout.JAVA_BYTE;
import static java.lang.foreign.ValueLayout.JAVA_INT;

import java.lang.foreign.Arena;
import java.lang.foreign.MemoryLayout.PathElement;
import java.lang.foreign.MemorySegment;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.stream.Stream;

public final class DdsSmokeTest {

    // Full 13-card holding bitmask (ranks 2..A), matching the Python fixtures.
    private static final int FULL_SUIT = 0x7FFC;
    private static final int RETURN_NO_FAULT = 1;

    // Double dummy table for the known deal, res_table[strain][hand] flattened
    // (5 strains x 4 hands). Cross-checked against the Python binding.
    private static final int[] EXPECTED_DD_TABLE = {
        13, 0, 13, 0,   // spades
        0, 13, 0, 13,   // hearts
        13, 0, 13, 0,   // diamonds
        0, 13, 0, 13,   // clubs
        0, 0, 0, 0,     // no-trump
    };

    // Byte offsets derived once from the public layouts (robust to edits).
    private static final long DEAL_TRUMP = Dds.DEAL.byteOffset(PathElement.groupElement("trump"));
    private static final long DEAL_FIRST = Dds.DEAL.byteOffset(PathElement.groupElement("first"));
    private static final long DEAL_REMAIN = Dds.DEAL.byteOffset(PathElement.groupElement("remainCards"));
    private static final long FT_CARDS = Dds.FUTURE_TRICKS.byteOffset(PathElement.groupElement("cards"));
    private static final long FT_SCORE = Dds.FUTURE_TRICKS.byteOffset(PathElement.groupElement("score"));
    private static final long INFO_SYSTEM_STRING =
            Dds.DDS_INFO.byteOffset(PathElement.groupElement("systemString"));
    private static final long INFO_NO_OF_THREADS =
            Dds.DDS_INFO.byteOffset(PathElement.groupElement("noOfThreads"));
    private static final long DTD_CARDS =
            Dds.DD_TABLE_DEAL.byteOffset(PathElement.groupElement("cards"));
    private static final long DTR_RES_TABLE =
            Dds.DD_TABLE_RESULTS.byteOffset(PathElement.groupElement("resTable"));
    private static final long PAR_SCORE =
            Dds.PAR_RESULTS.byteOffset(PathElement.groupElement("parScore"));

    public static void main(String[] args) throws Exception {
        Path library = locateLibrary();
        System.out.println("Loading DDS shared library: " + library);

        try (Dds dds = Dds.load(library); Arena arena = Arena.ofConfined()) {
            checkDdsInfo(dds, arena);
            checkSolveKnownDeal(dds, arena);
            checkSolveRejectsInvalidDeal(dds, arena);
            checkCalcDdTable(dds, arena);
            checkCalcPar(dds, arena);
        }
        System.out.println("DDS FFM smoke test passed.");
    }

    private static void checkDdsInfo(Dds dds, Arena arena) {
        MemorySegment info = arena.allocate(Dds.DDS_INFO);
        dds.getDdsInfo(info);

        String systemString = readCString(info, INFO_SYSTEM_STRING);
        int threads = info.get(JAVA_INT, INFO_NO_OF_THREADS);
        System.out.println("GetDDSInfo: threads=" + threads + " system=\"" + systemString + "\"");
        check(!systemString.isBlank(), "GetDDSInfo systemString should be non-empty");
        check(threads >= 1, "GetDDSInfo noOfThreads should be >= 1, was " + threads);
    }

    private static void checkSolveKnownDeal(Dds dds, Arena arena) {
        // North holds all spades, East all hearts, South all diamonds, West all
        // clubs; spades are trump and North leads. North ruffs every trick, so
        // the double dummy result is 13 tricks (score[0]) taking the ace of
        // spades (verified against the Python binding).
        MemorySegment deal = arena.allocate(Dds.DEAL);
        // Arena.allocate does not guarantee zeroed memory; clear the input
        // struct so currentTrick* and untouched remainCards entries are 0.
        deal.fill((byte) 0);
        deal.set(JAVA_INT, DEAL_TRUMP, 0); // trump = spades
        deal.set(JAVA_INT, DEAL_FIRST, 0); // first = North
        setRemain(deal, 0, 0, FULL_SUIT); // North spades
        setRemain(deal, 1, 1, FULL_SUIT); // East hearts
        setRemain(deal, 2, 2, FULL_SUIT); // South diamonds
        setRemain(deal, 3, 3, FULL_SUIT); // West clubs

        MemorySegment ctx = dds.createSolverContext();
        check(!ctx.equals(MemorySegment.NULL), "createSolverContext returned NULL");
        try {
            MemorySegment fut = arena.allocate(Dds.FUTURE_TRICKS);
            int rc = dds.solveBoard(ctx, deal, -1, 1, 1, fut);
            check(rc == RETURN_NO_FAULT, "dds_c_solve_board returned " + rc);

            int cards = fut.get(JAVA_INT, FT_CARDS);
            int topScore = fut.get(JAVA_INT, FT_SCORE);
            System.out.println("solve_board: cards=" + cards + " score[0]=" + topScore);
            check(cards == 1, "expected 1 candidate card, got " + cards);
            check(topScore == 13, "expected 13 tricks, got " + topScore);
        } finally {
            dds.destroySolverContext(ctx);
        }
    }

    private static void checkSolveRejectsInvalidDeal(Dds dds, Arena arena) {
        // trump = 5 is out of range (valid 0..4); the shim must surface the
        // solver's error code rather than succeeding or crashing.
        MemorySegment deal = arena.allocate(Dds.DEAL);
        deal.fill((byte) 0); // allocate is not guaranteed zero-initialized
        deal.set(JAVA_INT, DEAL_TRUMP, 5);
        setRemain(deal, 0, 0, FULL_SUIT);

        MemorySegment ctx = dds.createSolverContext();
        try {
            MemorySegment fut = arena.allocate(Dds.FUTURE_TRICKS);
            int rc = dds.solveBoard(ctx, deal, -1, 1, 1, fut);
            System.out.println("solve_board(invalid trump): rc=" + rc);
            check(rc != RETURN_NO_FAULT, "invalid deal should not return success, got " + rc);
        } finally {
            dds.destroySolverContext(ctx);
        }
    }

    private static void checkCalcDdTable(Dds dds, Arena arena) {
        // Same holdings as the solve fixture; DdTableDeal.cards is [hand][suit].
        MemorySegment tableDeal = arena.allocate(Dds.DD_TABLE_DEAL);
        // allocate is not guaranteed zero-initialized; clear the 12 unset cards.
        tableDeal.fill((byte) 0);
        setHolding(tableDeal, DTD_CARDS, 0, 0, FULL_SUIT);
        setHolding(tableDeal, DTD_CARDS, 1, 1, FULL_SUIT);
        setHolding(tableDeal, DTD_CARDS, 2, 2, FULL_SUIT);
        setHolding(tableDeal, DTD_CARDS, 3, 3, FULL_SUIT);

        MemorySegment ctx = dds.createSolverContext();
        try {
            MemorySegment results = arena.allocate(Dds.DD_TABLE_RESULTS);
            int rc = dds.calcDdTable(ctx, tableDeal, results);
            check(rc == RETURN_NO_FAULT, "dds_c_calc_dd_table returned " + rc);

            for (int i = 0; i < EXPECTED_DD_TABLE.length; i++) {
                int got = results.get(JAVA_INT, DTR_RES_TABLE + (long) i * Integer.BYTES);
                check(got == EXPECTED_DD_TABLE[i],
                        "resTable[" + i + "] expected " + EXPECTED_DD_TABLE[i] + ", got " + got);
            }
            System.out.println("calc_dd_table: 5x4 table matches expected.");
        } finally {
            dds.destroySolverContext(ctx);
        }
    }

    private static void checkCalcPar(Dds dds, Arena arena) {
        MemorySegment tableDeal = arena.allocate(Dds.DD_TABLE_DEAL);
        tableDeal.fill((byte) 0); // allocate is not guaranteed zero-initialized
        setHolding(tableDeal, DTD_CARDS, 0, 0, FULL_SUIT);
        setHolding(tableDeal, DTD_CARDS, 1, 1, FULL_SUIT);
        setHolding(tableDeal, DTD_CARDS, 2, 2, FULL_SUIT);
        setHolding(tableDeal, DTD_CARDS, 3, 3, FULL_SUIT);

        MemorySegment ctx = dds.createSolverContext();
        try {
            MemorySegment results = arena.allocate(Dds.DD_TABLE_RESULTS);
            MemorySegment par = arena.allocate(Dds.PAR_RESULTS);
            int rc = dds.calcPar(ctx, tableDeal, 0 /* vulnerable: none */, results, par);
            check(rc == RETURN_NO_FAULT, "dds_c_calc_par returned " + rc);

            String parScore = readCString(par, PAR_SCORE);
            System.out.println("calc_par: NS par score = \"" + parScore + "\"");
            check(!parScore.isBlank(), "calc_par NS par_score should be non-empty");
        } finally {
            dds.destroySolverContext(ctx);
        }
    }

    private static void setRemain(MemorySegment deal, int hand, int suit, int holding) {
        // remainCards[hand][suit], row-major with DDS_SUITS = 4 columns.
        setHolding(deal, DEAL_REMAIN, hand, suit, holding);
    }

    private static void setHolding(MemorySegment struct, long base, int hand, int suit, int holding) {
        // [hand][suit] array, row-major with DDS_SUITS = 4 columns.
        struct.set(JAVA_INT, base + (long) (hand * 4 + suit) * Integer.BYTES, holding);
    }

    private static String readCString(MemorySegment struct, long offset) {
        // Read a fixed char[] field as a NUL-terminated string. Bound the scan
        // to the segment so a missing terminator (or a bad offset) fails with a
        // clear error instead of reading out of bounds.
        StringBuilder sb = new StringBuilder();
        long limit = struct.byteSize();
        for (long i = offset; i < limit; i++) {
            byte b = struct.get(JAVA_BYTE, i);
            if (b == 0) {
                return sb.toString();
            }
            sb.append((char) (b & 0xFF));
        }
        throw new IllegalStateException(
                "unterminated C string at offset " + offset + " within " + limit + " bytes");
    }

    private static Path locateLibrary() throws Exception {
        String prop = System.getProperty("dds.library.path");
        if (prop != null) {
            Path p = Path.of(prop);
            if (Files.exists(p)) {
                return p;
            }
        }
        // Fall back to searching the runfiles tree for the built artifact.
        // Bound the walk depth so a missing/incorrect dds.library.path does not
        // send us traversing an arbitrarily large checkout or runfiles tree.
        String[] names = {"libdds.dylib", "libdds.so", "dds.dll"};
        Path root = Path.of(System.getProperty("user.dir"));
        int maxDepth = 12;
        try (Stream<Path> walk = Files.walk(root, maxDepth)) {
            return walk.filter(Files::isRegularFile)
                    .filter(p -> {
                        String n = p.getFileName().toString();
                        for (String name : names) {
                            if (n.equals(name)) {
                                return true;
                            }
                        }
                        return false;
                    })
                    .findFirst()
                    .orElseThrow(() -> new IllegalStateException(
                            "DDS shared library not found (dds.library.path=" + prop + ")"));
        }
    }

    private static void check(boolean condition, String message) {
        if (!condition) {
            throw new AssertionError(message);
        }
    }

    private DdsSmokeTest() {
    }
}

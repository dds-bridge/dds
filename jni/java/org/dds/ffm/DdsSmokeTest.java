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

    public static void main(String[] args) throws Exception {
        Path library = locateLibrary();
        System.out.println("Loading DDS shared library: " + library);

        try (Dds dds = Dds.load(library); Arena arena = Arena.ofConfined()) {
            checkDdsInfo(dds, arena);
            checkSolveKnownDeal(dds, arena);
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

    private static void setRemain(MemorySegment deal, int hand, int suit, int holding) {
        // remainCards[hand][suit], row-major with DDS_SUITS = 4 columns.
        deal.set(JAVA_INT, DEAL_REMAIN + (long) (hand * 4 + suit) * Integer.BYTES, holding);
    }

    private static String readCString(MemorySegment struct, long offset) {
        // Read a fixed char[] field as a NUL-terminated string.
        StringBuilder sb = new StringBuilder();
        for (long i = offset; ; i++) {
            byte b = struct.get(JAVA_BYTE, i);
            if (b == 0) {
                break;
            }
            sb.append((char) (b & 0xFF));
        }
        return sb.toString();
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
        String[] names = {"libdds.dylib", "libdds.so", "dds.dll"};
        Path root = Path.of(System.getProperty("user.dir"));
        try (Stream<Path> walk = Files.walk(root)) {
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

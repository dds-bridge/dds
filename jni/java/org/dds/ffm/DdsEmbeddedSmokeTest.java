/*
   DDS, a bridge double dummy solver.

   Smoke test for the packaged jar: load the native library via
   Dds.loadEmbedded() (from the jar's classpath resource, not a filesystem
   path) and solve a known deal, proving the embed -> extract -> load chain.
   The broader shim surface is covered by DdsSmokeTest; this test only varies
   how the library is located.

   Runs as a plain main (use_testrunner = False): success exits 0, any failure
   throws and fails the test.

   See LICENSE and README.
*/

package org.dds.ffm;

import static java.lang.foreign.ValueLayout.JAVA_INT;

import java.lang.foreign.Arena;
import java.lang.foreign.MemoryLayout.PathElement;
import java.lang.foreign.MemorySegment;

public final class DdsEmbeddedSmokeTest {

    private static final int FULL_SUIT = 0x7FFC;
    private static final int RETURN_NO_FAULT = 1;

    private static final long DEAL_TRUMP = Dds.DEAL.byteOffset(PathElement.groupElement("trump"));
    private static final long DEAL_REMAIN = Dds.DEAL.byteOffset(PathElement.groupElement("remainCards"));
    private static final long FT_CARDS = Dds.FUTURE_TRICKS.byteOffset(PathElement.groupElement("cards"));
    private static final long FT_SCORE = Dds.FUTURE_TRICKS.byteOffset(PathElement.groupElement("score"));
    private static final long INFO_MAJOR = Dds.DDS_INFO.byteOffset(PathElement.groupElement("major"));
    private static final long INFO_MINOR = Dds.DDS_INFO.byteOffset(PathElement.groupElement("minor"));

    public static void main(String[] args) {
        try (Dds dds = Dds.loadEmbedded(); Arena arena = Arena.ofConfined()) {
            checkCoordinateMatchesLibrary(dds, arena);
            // North all spades (trump), East hearts, South diamonds, West clubs;
            // North ruffs every trick -> 13 tricks. Cross-checked in DdsSmokeTest.
            MemorySegment deal = arena.allocate(Dds.DEAL);
            // Arena.allocate is not guaranteed zero-initialized; clear the input
            // struct so currentTrick* and untouched remainCards entries are 0.
            deal.fill((byte) 0);
            deal.set(JAVA_INT, DEAL_TRUMP, 0);
            setRemain(deal, 0, 0, FULL_SUIT);
            setRemain(deal, 1, 1, FULL_SUIT);
            setRemain(deal, 2, 2, FULL_SUIT);
            setRemain(deal, 3, 3, FULL_SUIT);

            MemorySegment ctx = dds.createSolverContext();
            try {
                MemorySegment fut = arena.allocate(Dds.FUTURE_TRICKS);
                int rc = dds.solveBoard(ctx, deal, -1, 1, 1, fut);
                check(rc == RETURN_NO_FAULT, "dds_c_solve_board returned " + rc);
                int cards = fut.get(JAVA_INT, FT_CARDS);
                int topScore = fut.get(JAVA_INT, FT_SCORE);
                System.out.println("embedded solve_board: cards=" + cards + " score[0]=" + topScore);
                check(cards == 1, "expected 1 candidate card, got " + cards);
                check(topScore == 13, "expected 13 tricks, got " + topScore);
            } finally {
                dds.destroySolverContext(ctx);
            }
        }
        System.out.println("DDS embedded FFM smoke test passed.");
    }

    private static void checkCoordinateMatchesLibrary(Dds dds, Arena arena) {
        // Guard against the published Maven version silently diverging from the
        // embedded native library: the coordinate's major.minor must match the
        // library's GetDDSInfo. Patch may differ (package vs C-API versioning).
        String coordinate = System.getProperty("dds.coordinate.version");
        if (coordinate == null || coordinate.isBlank()) {
            return; // only enforced when the build passes the coordinate version
        }
        MemorySegment info = arena.allocate(Dds.DDS_INFO);
        dds.getDdsInfo(info);
        int libMajor = info.get(JAVA_INT, INFO_MAJOR);
        int libMinor = info.get(JAVA_INT, INFO_MINOR);
        String[] parts = coordinate.split("\\.");
        int coordMajor = Integer.parseInt(parts[0]);
        int coordMinor = parts.length > 1 ? Integer.parseInt(parts[1]) : 0;
        System.out.println("version check: coordinate=" + coordinate
                + " library=" + libMajor + "." + libMinor);
        check(libMajor == coordMajor && libMinor == coordMinor,
                "Maven coordinate " + coordinate + " major.minor does not match library "
                        + libMajor + "." + libMinor);
    }

    private static void setRemain(MemorySegment deal, int hand, int suit, int holding) {
        deal.set(JAVA_INT, DEAL_REMAIN + (long) (hand * 4 + suit) * Integer.BYTES, holding);
    }

    private static void check(boolean condition, String message) {
        if (!condition) {
            throw new AssertionError(message);
        }
    }

    private DdsEmbeddedSmokeTest() {
    }
}

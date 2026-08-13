# dtest Program

The `dtest` program is a test harness for the Double Dummy Solver (DDS) library. It is designed to verify the correctness of the solver by running it against a series of predefined test cases.

## Test Cases

The test cases are located in the `hands` directory. Each `.txt` file in this directory represents one or more bridge deals and their expected outcomes. The program parses these files, executes the solver for each scenario, and compares the results with the provided expectations.

### Input File Format

The input files use a specific format with keywords to define the test parameters for each deal. The first line is `NUMBER N`, where `N` is the count of deals in the file. Each deal is then a block of lines in this order:

- **`PBN`**: The deal definition in Portable Bridge Notation, describing the cards held by each of the four players (North, East, South, West), plus dealer, vulnerability, trump, and leader.
- **`FUT`**: Expected `SolveBoard` future-tricks result for the deal (card count plus suit/rank/equals/score arrays).
- **`TABLE`**: Expected double-dummy table: 20 integers, `res_table[strain][hand]` for 5 strains (♠♥♦♣NT) × 4 seats (N/E/S/W).
- **`PAR`**: Expected par scores and contract strings for NS and EW views.
- **`PAR2`**: Expected dealer-par result (score plus one or more contract strings).
- **`PLAY`**: A specific sequence of cards to be played, used to test the solver's analysis of a particular line of play.
- **`TRACE`**: Expected `AnalysePlay` trick counts after each card in `PLAY`.

## Usage

The `dtest` program is typically run from the command line, taking an input file from the `hands` directory as an argument. It processes the test cases within the file and reports any discrepancies between the solver's output and the expected results.

For example:

```bash
./dtest ../hands/list1.txt
```

This command would run the test cases defined in `list1.txt` and check the DDS library's calculations.
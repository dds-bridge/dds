"""Bazel-native regression tests for calc_all_tables_pbn table row semantics."""

from dds3 import calc_all_tables_pbn


def test_nt_row_and_filter_semantics() -> None:
    """NT is row index 4 and filtered strains are returned as zero-filled rows."""
    deals = ["N:Q87.K932.QJT32.7 AKJ9632.J84.6.Q5 .AQT765.K87.J962 T54..A954.AKT843"]

    all_strains = calc_all_tables_pbn(deals, trump_filter=[0, 0, 0, 0, 0])
    all_rows = all_strains["tables"][0]["res_table"]

    nt_only = calc_all_tables_pbn(deals, trump_filter=[1, 1, 1, 1, 0])
    nt_only_rows = nt_only["tables"][0]["res_table"]

    exclude_nt = calc_all_tables_pbn(deals, trump_filter=[0, 0, 0, 0, 1])
    exclude_nt_rows = exclude_nt["tables"][0]["res_table"]

    assert len(all_rows) == 5
    assert len(nt_only_rows) == 5
    assert len(exclude_nt_rows) == 5

    assert nt_only_rows[4] == all_rows[4]
    assert all(value == 0 for row in nt_only_rows[:4] for value in row)

    assert all(value == 0 for value in exclude_nt_rows[4])
    for strain in range(4):
        assert exclude_nt_rows[strain] == all_rows[strain]

"""Simple smoke tests for calc_par functions (no pytest required)."""

from dds3 import calc_par_from_table, calc_all_tables_pbn


def test_calc_par_basic() -> None:
    """Basic smoke test for calc_par using PBN hand."""
    # Use a known valid hand in PBN format
    pbn_deals = ["N:QJ6.K652.J85.T98 873.J97.AT764.Q4 K5.T83.KQ9.A7652 AT942.AQ4.32.KJ3"]
    
    # First, get the DD table for this hand using calc_all_tables_pbn
    tables_result = calc_all_tables_pbn(pbn_deals, mode=-1)  # mode=-1 disables par in batch calc
    
    # Get the DD table
    assert "tables" in tables_result, "calc_all_tables_pbn should return tables"
    assert len(tables_result["tables"]) > 0, "Should have at least one table"
    
    table_result = tables_result["tables"][0]
    
    # Now test calc_par_from_table
    par_result = calc_par_from_table(table_result, vulnerable=0)
    assert isinstance(par_result, dict), "calc_par_from_table should return a dict"
    assert "par_score" in par_result, "par_results should have par_score"
    assert "par_contracts_string" in par_result, "par_results should have par_contracts_string"
    print("✓ calc_par basic test passed")


def test_calc_par_vulnerabilities() -> None:
    """Test calc_par_from_table with different vulnerabilities."""
    # Use calc_all_tables_pbn to get a valid DD table
    pbn_deals = ["N:QJ6.K652.J85.T98 873.J97.AT764.Q4 K5.T83.KQ9.A7652 AT942.AQ4.32.KJ3"]
    tables_result = calc_all_tables_pbn(pbn_deals, mode=-1)
    
    table_result = tables_result["tables"][0]
    
    for vuln in [0, 1, 2, 3]:
        par_result = calc_par_from_table(table_result, vulnerable=vuln)
        assert isinstance(par_result, dict), f"calc_par_from_table with vulnerable={vuln} should return a dict"
        assert "par_score" in par_result, f"calc_par_from_table with vulnerable={vuln} should have par_score"
    print("✓ calc_par vulnerability test passed")


def test_calc_par_from_table_basic() -> None:
    """Basic smoke test for calc_par_from_table."""
    # Use calc_all_tables_pbn to get a valid DD table
    pbn_deals = ["N:QJ6.K652.J85.T98 873.J97.AT764.Q4 K5.T83.KQ9.A7652 AT942.AQ4.32.KJ3"]
    tables_result = calc_all_tables_pbn(pbn_deals, mode=-1)
    
    assert "tables" in tables_result, "calc_all_tables_pbn should return tables"
    table_result = tables_result["tables"][0]
    
    # Now compute par from that table
    par_result = calc_par_from_table(table_result, vulnerable=0)
    assert isinstance(par_result, dict), "calc_par_from_table should return a dict"
    assert "par_score" in par_result, "par_results should have par_score"
    assert "par_contracts_string" in par_result, "par_results should have par_contracts_string"
    print("✓ calc_par_from_table basic test passed")


def test_calc_par_consistency() -> None:
    """Test that multiple calls with same input give consistent results."""
    # Use calc_all_tables_pbn to get a valid DD table
    pbn_deals = ["N:QJ6.K652.J85.T98 873.J97.AT764.Q4 K5.T83.KQ9.A7652 AT942.AQ4.32.KJ3"]
    tables_result = calc_all_tables_pbn(pbn_deals, mode=-1)
    
    table_result = tables_result["tables"][0]
    
    # Call twice
    par_result1 = calc_par_from_table(table_result, vulnerable=0)
    par_result2 = calc_par_from_table(table_result, vulnerable=0)
    
    # Results should match
    assert par_result1["par_score"] == par_result2["par_score"], \
        "Par scores from repeated calls should match"
    print("✓ calc_par consistency test passed")


if __name__ == "__main__":
    test_calc_par_basic()
    test_calc_par_vulnerabilities()
    test_calc_par_from_table_basic()
    test_calc_par_consistency()
    print("\n✓ All calc_par smoke tests passed!")

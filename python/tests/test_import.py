from dds3 import api_root


def test_import_and_api_root() -> None:
    assert api_root() == "dds.hpp"

from dds3 import api_root
from dds3 import module_name


def test_import_and_api_root() -> None:
    assert api_root() == "dds.hpp"
    assert module_name() == "_dds3"

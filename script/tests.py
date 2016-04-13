import subunit

class ShellTests(subunit.ExecTestCase):
    """Run some tests from the C code base"""

    def test_group_one(self):
        '../build/test/check-all'

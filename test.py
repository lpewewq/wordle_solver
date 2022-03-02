import sys
import os
import json
import words


def score_to_string(score: int) -> str:
    """ 3 base encoded score to string """

    def decode(x):
        return "⬛" if x == 0 else "🟨" if x == 1 else "🟩"

    result = ""
    d = score
    for _ in range(5):
        d, m = divmod(d, 3)
        result += decode(m)
    return result


def score(test_word: str, hidden_word: str) -> int:
    """ Black -> 0, Yellow -> 1, Green -> 2 """
    result = 0
    crossed = set()
    for i, (t, h) in enumerate(zip(test_word, hidden_word)):
        if t == h:
            result += 2 * (3 ** i)
            crossed.add(i)

    for i, (t, h1) in enumerate(zip(test_word, hidden_word)):
        if t != h1:
            for j, h2 in enumerate(hidden_word):
                if t == h2 and j not in crossed:
                    result += 3 ** i
                    crossed.add(j)
                    break
    return result


class DecisionTreeValidator:
    def __init__(self, hidden_words, test_words) -> None:
        self.hidden_words = hidden_words
        self.test_words = test_words
        n_hidden = len(self.hidden_words)
        n_test = len(self.test_words)
        print(f"test n_hidden={n_hidden} n_test={n_test}")

        code = os.system(f"./release {n_hidden} {n_test}")
        assert code == 0, f"Solver return code {code}"

        with open("result.json", "r") as f:
            self.result = json.load(f)
        assert self.result["n_hidden"] == n_hidden
        assert self.result["n_test"] == n_test

        self.validate_node(self.result["decision_tree"])
        total = 0
        for hidden_word in self.hidden_words:
            total += self.play(hidden_word, self.result["decision_tree"])
        assert total == self.result["total"]
        if n_hidden > 0:
            assert (total / n_hidden - self.result["average"]) <= 1e-4

        key = f"{n_hidden:0>5}, {n_test:0>5}"
        with open("expected_results.json", "r") as f:
            expected_results = json.load(f)
        if key in expected_results:
            assert expected_results[key] == self.result["total"]
        else:
            expected_results[key] = self.result["total"]
            with open("expected_results.json", "w") as f:
                json.dump(expected_results, f, indent=4, sort_keys=True)

    def play(self, hidden_word, node):
        guess = node["guess"]
        if guess == hidden_word:
            return 1
        key = score_to_string(score(guess, hidden_word))
        assert key in node["branches"]
        return 1 + self.play(hidden_word, node["branches"][key])

    def validate_node(self, node):
        if node is None:
            return
        guess = node["guess"]
        beta = node["beta"]
        branches = node["branches"]
        max_depth = node["max_depth"]

        assert beta > 0
        assert guess in self.test_words
        if branches is None:
            assert guess in self.hidden_words
            assert beta == 1
            assert max_depth == 1
            return 1

        sub_max_depth = 0
        for key, item in branches.items():
            assert key not in ["🟨🟩🟩🟩🟩", "🟩🟨🟩🟩🟩", "🟩🟩🟨🟩🟩", "🟩🟩🟩🟨🟩", "🟩🟩🟩🟩🟨", "🟩🟩🟩🟩🟩"]
            self.validate_node(item)
            sub_max_depth = max(sub_max_depth, item["max_depth"])
        assert max_depth == 1 + sub_max_depth


def test(n_hidden, n_test):
    assert 0 <= n_hidden <= words.N_HIDDEN
    assert 0 <= n_test <= words.N_TEST
    assert n_hidden <= n_test

    hidden_words = words.hidden_words[:n_hidden]
    test_words = words.test_words[:n_test]
    DecisionTreeValidator(hidden_words, test_words)


if __name__ == "__main__":
    n_hidden = words.N_HIDDEN
    n_test = words.N_TEST
    if len(sys.argv) > 1:
        n_hidden = int(sys.argv[1])
    if len(sys.argv) > 2:
        n_test = int(sys.argv[2])

    code = os.system(f"make clean && make release")
    assert code == 0
    test(n_hidden, n_test)

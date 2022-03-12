import sys
import os
import json
import words
from multiset import Multiset

INF = 18446744073709551615


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


def test(n_hidden, n_test, hard_mode):
    assert 0 <= n_hidden <= words.N_HIDDEN
    assert 0 <= n_test <= words.N_TEST
    assert n_hidden <= n_test

    hidden_words = words.hidden_words[:n_hidden]
    test_words = words.test_words[:n_test]

    hidden_words = hidden_words
    test_words = test_words
    n_hidden = len(hidden_words)
    n_test = len(test_words)
    print(f"test n_hidden={n_hidden} n_test={n_test}")

    result_file = f"result_{n_hidden}_{n_test}_{hard_mode}.json"
    call = f"./release {int(hard_mode)} {n_hidden} {n_test} {result_file}"
    if hard_mode:
        call += " hard"
    code = os.system(call)
    assert code == 0, f"Solver return code {code}"

    with open(result_file, "r") as f:
        result = json.load(f)

    if n_hidden == 0:
        assert result is None
        return

    assert result["n_hidden"] == n_hidden
    assert result["n_test"] == n_test

    def validate_node(node, depth, validation_test_words):
        assert node is not None
        n_hidden = node["n_hidden"]
        guess = node["guess"]
        total = node["total"]
        branches = node["branches"]
        worst_case = node["worst_case"]
        best_case = node["best_case"]
        average_case = node["average_case"]

        if n_hidden == 0:
            assert depth == 0
            assert total == 0
            assert guess is None
            assert branches is None
            assert worst_case == 0
            assert best_case >= INF
            assert average_case >= INF
            return

        assert total > 0
        assert guess is not None
        assert guess in validation_test_words

        if branches is None:
            assert guess in hidden_words
            assert total == 1
            assert worst_case == 1
            assert best_case == 1
            assert average_case == 1
            return

        sub_worst_case = 0
        sub_best_case = 1 if guess in hidden_words else INF
        for key, item in branches.items():
            assert key not in ["🟨🟩🟩🟩🟩", "🟩🟨🟩🟩🟩", "🟩🟩🟨🟩🟩", "🟩🟩🟩🟨🟩", "🟩🟩🟩🟩🟨", "🟩🟩🟩🟩🟩"]
            sub_test_words = validation_test_words
            if hard_mode:
                exact_match = [(i, guess[i]) for i in range(5) if key[i] == "🟩"]
                include_match = Multiset(guess[i] for i in range(5) if key[i] != "⬛")
                sub_test_words = [
                    test_word
                    for test_word in sub_test_words
                    if all(test_word[i] == c for i, c in exact_match) and include_match <= Multiset(test_word)
                ]
            validate_node(item, depth + 1, sub_test_words)
            sub_worst_case = max(sub_worst_case, 1 + item["worst_case"])
            sub_best_case = min(sub_best_case, 1 + item["best_case"])
        assert worst_case == sub_worst_case
        assert best_case == sub_best_case
        assert best_case <= average_case <= worst_case

    validate_node(result, 0, test_words)

    def play(hidden_word, node):
        guess = node["guess"]
        if guess == hidden_word:
            return 1
        key = score_to_string(score(guess, hidden_word))
        assert node["branches"] is not None
        assert key in node["branches"]
        return 1 + play(hidden_word, node["branches"][key])

    total = 0
    for hidden_word in hidden_words:
        total += play(hidden_word, result)
    assert total == result["total"]
    if n_hidden > 0:
        assert (total / n_hidden - result["average_case"]) <= 1e-4


if __name__ == "__main__":
    n_hidden = words.N_HIDDEN
    n_test = words.N_TEST
    hard_mode = False
    if len(sys.argv) > 1:
        hard_mode = sys.argv[1] == "1"
    if len(sys.argv) > 2:
        n_hidden = int(sys.argv[2])
    if len(sys.argv) > 3:
        n_test = int(sys.argv[3])

    code = os.system(f"make clean && make release")
    assert code == 0
    test(n_hidden, n_test, hard_mode)

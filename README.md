# sorted-set (C++)

[tatyam-prime/SortedSet](https://github.com/tatyam-prime/SortedSet) の C++17 移植です。
バケットを分割して管理するヘッダーオンリーライブラリです。外部依存はありません。
名前空間は `sorted_set` です。

- `SortedSet<T, Compare>`: 重複を除く順序付き集合
- `SortedMultiset<T, Compare>`: 重複を許す順序付き多重集合
- `BucketList<T>`: 挿入順を保持するバケット方式のリスト

## 導入

必要な型のファイルをコピーし、C++17 以降でコンパイルしてください。
各ファイルは標準ライブラリだけに依存し、他の実装ファイルは不要です。
ヘッダーの内容全体を提出コードの先頭へ直接貼り付ける場合、そのヘッダーへの `#include` は不要です。
複数種類を同じコードに貼り付けても利用できます。

| 型 | 単独で使えるファイル |
| --- | --- |
| `SortedSet` | [include/sorted_set.hpp](include/sorted_set.hpp) |
| `SortedMultiset` | [include/sorted_multiset.hpp](include/sorted_multiset.hpp) |
| `BucketList` | [include/bucket_list.hpp](include/bucket_list.hpp) |

公開関数には Doxygen 形式の `///` コメントと `@par Examples` / `@code{.cpp}` の使用例を付けています。
各例はそのままコンパイルできるプログラムで、`python3 tests/doc_examples.py` がコメントから抽出して実行します。

```cpp
#include "sorted_set.hpp"
#include "sorted_multiset.hpp"
#include <cassert>

int main() {
    sorted_set::SortedSet<int> s{3, 1, 3, 2};
    s.add(4);
    assert(s.lt(3) && *s.lt(3) == 2);
    assert(s.index(3) == 2);
    assert(s[-1] == 4);
    assert(s.pop() == 4);

    sorted_set::SortedMultiset<int> m{2, 2, 1};
    m.discard(2); // 1 個だけ削除
    assert(m.count(2) == 1);
}
```

```sh
g++ -std=c++17 -O2 -Iinclude examples/basic.cpp -o /tmp/sorted-set-example
/tmp/sorted-set-example
```

CMake のプロジェクトからは `add_subdirectory(path/to/sorted-set-cpp)` と
`target_link_libraries(your_target PRIVATE sorted_set)` で利用できます。

## ドキュメント・検証

- [API リファレンス](docs/API.md): 全操作、計算量、比較関数、例外、無効化規則
- [実行可能な使用例](examples/basic.cpp)

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
python3 tests/standalone.py
python3 tests/doc_examples.py
./build/sorted_set_example
```

テストは `std::set` / `std::multiset` / `std::vector` とのランダム比較、バケットの境界、負の添字、カスタム比較関数、move-only 型を含みます。
GitHub Actions では GCC / Clang と AddressSanitizer / UndefinedBehaviorSanitizer を使用します。

## 出典とライセンス

移植元: [tatyam-prime/SortedSet](https://github.com/tatyam-prime/SortedSet/tree/9a205c686c225c2b083c4852516da34a6ca1f4c2)
（コミット `9a205c686c225c2b083c4852516da34a6ca1f4c2`）。
元実装の `BUCKET_RATIO = 16`、`SPLIT_RATIO = 24` と分割方式を使用しています。
本実装も [Unlicense](LICENSE) です。

# C++ API

```cpp
#include "sorted_set.hpp"
using sorted_set::SortedSet;
using sorted_set::SortedMultiset;
using sorted_set::BucketList;
```

## 型・共通規則

- `SortedSet<T, Compare = std::less<T>>`
- `SortedMultiset<T, Compare = std::less<T>>`
- `BucketList<T>`

集合型は `SortedCollection<T, bool Multi, Compare>` のエイリアスです。
`Compare` は strict weak ordering（狭義弱順序）を満たす必要があります。
重複は `!comp(a, b) && !comp(b, a)` で判定します。
近傍検索・順位はこの比較順序に従います。
例えば `SortedSet<int, std::greater<int>>` は降順になり、その `lt(x)` は数値上では `x` より大きい側を探します。
格納後に比較結果を変えてはいけません。

型 `T` は、使用する vector 操作に応じて move 構築・move 代入などを満たす必要があります。
コピー構築・`copy()` はコピー可能な `T` が必要ですが、`add` / `pop` は move-only 型でも使えます。
`BucketList` の値検索とコンテナの等値比較には `T` の `operator==` が必要です。

すべての変更操作は、返したポインタ・参照・イテレータを無効化するものとして扱ってください。
変更後は再取得してください。走査しながらの追加・削除には対応しません。
集合の添字とイテレータは常に `const T&` を返し、順序を壊す変更を防ぎます。

## 構築

```cpp
SortedSet<int> empty;
SortedSet<int> s{3, 1, 3};
SortedMultiset<int> m(std::vector<int>{3, 1, 3});
SortedSet<int> from_iter(first, last);
SortedSet<int, std::greater<int>> descending({1, 2, 3}, std::greater<int>{});
BucketList<int> list{3, 1, 2};
```

集合は必要な場合だけソートし、`SortedSet` は重複を除きます。
`BucketList` は入力順を保存します。
vector 引数に `std::move(values)` を渡すと所有権を移せます。

## SortedSet / SortedMultiset

| 操作 | 内容 |
| --- | --- |
| `size() -> std::size_t` / `empty() -> bool` | 要素数 / 空判定 |
| `contains(const T& x) -> bool` | 存在判定 |
| `add(T x) -> bool` | 新規追加なら `true`。集合で既存なら `false`。多重集合は常に追加して `true` |
| `discard(const T& x) -> bool` | 1 個だけ削除。なければ `false` |
| `lt(x) -> const T*` | `x` 未満の最大要素 |
| `le(x) -> const T*` | `x` 以下の最大要素 |
| `gt(x) -> const T*` | `x` より大きい最小要素 |
| `ge(x) -> const T*` | `x` 以上の最小要素 |
| `index(x) -> std::size_t` | `x` 未満の要素数。存在しない値にも使用可能 |
| `index_right(x) -> std::size_t` | `x` 以下の要素数 |
| `count(x) -> std::size_t` | `x` の個数（集合にも提供） |
| `at(std::ptrdiff_t i) -> const T&` / `s[i]` | 添字で参照。両方とも範囲を検査 |
| `pop(std::ptrdiff_t i = -1) -> T` | 指定位置を削除して値を返す。省略時は末尾 |
| `begin()` / `end()` / `cbegin()` / `cend()` | 読み取り専用の双方向イテレータ |
| `rbegin()` / `rend()` | 逆順イテレータ |
| `clear()` | 空にする |
| `buckets()` | `const std::vector<std::vector<T>>&` |
| コピー構築・代入 | 独立したコピー |
| move 構築・代入 | 所有権を移動。移動元は空の状態で再利用可能 |
| `==` / `!=` | 値の並びを比較。バケット構成には依存しない |

近傍が存在しないときは `nullptr` です。逆参照の前に確認してください。
添字は符号付きで、`0` が先頭、`-1` が末尾、`-size()` 相当が先頭です。
範囲外の `at` / `operator[]` / `pop` は `std::out_of_range` を送出します。
`size()` は符号なしなので、負の添字を作る際はまず `std::ptrdiff_t` に変換してください。

## BucketList

| 操作 | 内容 |
| --- | --- |
| `size()` / `empty()` / `buckets()` | 要素数 / 空判定 / 読み取り専用バケット |
| `at(i)` / `s[i]` | 添字参照。非 const コンテナでは `T&` で変更可能 |
| `insert(std::ptrdiff_t i, T x)` | `i` 番目の直前に挿入 |
| `append(T x)` | 末尾に追加 |
| `extend(first, last)` | イテレータ範囲を末尾に追加。自分自身のイテレータは渡さない |
| `pop(std::ptrdiff_t i = -1) -> T` | 添字で削除。省略時は末尾 |
| `contains(x) -> bool` / `count(x) -> std::size_t` | 存在判定 / 個数 |
| `index(x) -> std::size_t` | 最初の一致位置。集合の `index` とは異なる |
| `remove(x)` | 最初の一致を 1 個削除 |
| `reverse()` | その場で反転 |
| `clear()` / `copy()` | 空にする / 独立コピー |
| `begin()` / `end()` / `cbegin()` / `cend()` / `rbegin()` / `rend()` | 読み取り専用走査 |
| コピー・move・`==`・`!=` | 集合と同様 |

空でないリストの `insert` は `-size..=size` の範囲で有効です。
`insert(-1, x)` は末尾要素の直前、`insert(size, x)` は末尾に挿入します。
空の場合は元実装に合わせて `0` と `-1` のみ有効です。
範囲外では `std::out_of_range`、`index` / `remove` で値が見つからなければ `std::invalid_argument` を送出します。
これらの検出は変更前に行います。要素操作・比較・メモリ確保からの例外についてはトランザクション的なロールバックを保証しません。

## 計算量

`N` を要素数、`B` をバケット数、`K` を最大バケット長とします。

| 操作 | 計算量 |
| --- | --- |
| 集合の構築 | ソート済みなら O(N)、それ以外は O(N log N) |
| リストの構築 | O(N) |
| 要素数・空判定 | O(1) |
| 集合の存在・近傍・順位・個数 | O(B + log K) |
| 添字参照 | O(B)。負の添字は末尾から探索 |
| 挿入・添字削除・集合の値による削除 | O(B + K)。vector の再確保は償却 |
| リストの値検索・個数・値による削除 | O(N) |
| 全走査・比較・コピー・反転 | O(N) |
| メモリ | O(N) |

初期バケット数は `ceil(sqrt(N / 16))` です。
バケット長が `24 * B` を超えたら二分割し、空になったバケットは取り除きます。
通常は O(√N) 程度ですが、元実装と同様に削除時の全体再構築は行わないため、履歴により分布が偏ります。
現在の `N` に対する厳密な最悪 O(√N) は保証しません。

## Python 版との違い

- Python の近傍検索の `None` は `nullptr`、`IndexError` は `std::out_of_range`、`ValueError` は `std::invalid_argument` です。
- `SortedMultiset.add` は `None` ではなく `true` を返します。
- `reversed(s)` は逆順イテレータに対応します。
- 内部バケットは読み取り専用です。スライス、Python 形式の文字列化は提供しません。
- `std::set` とは異なり、添字アクセスや順位取得を提供します。比較性能が `std::set` より良いことを保証するものではありません。

# ディレクトリ名に読み仮名を振る
## 動作環境
- C++ 26
- GCC 16.1.0 or later
- cmake 3.25 or later
- libcurl
- <a href="https://github.com/clever-elsie/jpp">jpp</a>
- port 11434 で ollamaが動いている必要がある
- ollamaではgemma4:e4bと26bが使えるとデフォルト動作可能．別のモデルを使う場合はコマンドラインで指定
- g++のパスと標準ライブラリのパスはcmakeを適宜書き換えてください．
- デフォルトでは/usr/local/bin/g++, /usr/local/bin/lib64を使うようになっています．
- jppのパスも指定してください．デフォルトは私の入れてる場所なので多分使えないです．


## usage
### ビルド
```sh
make
```

### 実行
```
build/furigana /path/to/target/directory
```
対象ディレクトリを再帰的に走査してディレクトリに読み仮名を振る．

読み仮名は"元のディレクトリ名《もとのでぃれくとりめい》"の形式で振られる．

この形式は<a href="https://github.com/clever-elsie/archive">archive</a>の対応形式．

### モデル指定
読み仮名生成モデル
```sh
-g "gemma4:e4b"
--generate "gemma4:26b"
--generate-model "gemma4:31b"
```

読み仮名チェックモデル
```sh
-c "gemma4:e4b"
--check "gemma4:26b"
--check-model "gemma4:31b"
```


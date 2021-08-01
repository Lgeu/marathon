# marathon
競プロの

### 単調増加関数を作るやつ
https://lgeu.github.io/marathon/tools/monotonically_increasing_function.html

### GCE の環境構築

`source env.sh` で実行する。

```bash
sudo apt update

# g++ とか入れる
sudo apt install zip unzip build-essential -y

# Rust の環境構築
# curl -s 進捗を表示しない -S エラーは表示する -L リダイレクトがあったらリダイレクト先に行く
# sh -s 標準入力でコマンドを指定
curl https://sh.rustup.rs -sSf | sh -s -- -y --default-toolchain stable
source .bashrc
# インストールできたか確認
cargo --version

# Python の環境構築 (Anaconda を使う、適宜最新版を確認 https://www.anaconda.com/products/individual)
# curl -O ファイルに保存
curl https://repo.anaconda.com/archive/Anaconda3-2021.05-Linux-x86_64.sh -sSLO
# Anaconda をインストールするスクリプトの -b は yes の入力を省略したりするオプション
sh Anaconda3-2021.05-Linux-x86_64.sh -b
rm Anaconda3-2021.05-Linux-x86_64.sh
anaconda3/bin/conda init
source .bashrc
# インストールできたか確認
conda --version
# Optuna のインストール
conda install -c conda-forge optuna -y
```

jupyter notebook の実行

```bash
jupyter notebook --ip=*
```

### TODO
- Hungry Geese で使った NN を整理する
  - 学習 https://github.com/Lgeu/hungry_geese/blob/master/nagiss/rl/notebooks/038_train/038_train.ipynb
  - 推論 https://github.com/Lgeu/hungry_geese/blob/master/KKT89/src/Evaluator2.hpp

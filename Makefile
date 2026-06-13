# Todo CLI — 多语言统一 Makefile
# 用法：make <target> LANG=rust   (默认不指定时构建所有语言)
#       make run ARGS="list"      (传递参数给运行目标)

LANG ?= all
ARGS ?=

# ─── C++ ──────────────────────────────────────────────────────────
CPP_BUILD_DIR = cpp/build
CPP_BIN      = $(CPP_BUILD_DIR)/todo

.PHONY: cpp-build cpp-run cpp-test cpp-clean

cpp-build:
	@echo "=== C++: 构建 ==="
	cmake -S cpp -B $(CPP_BUILD_DIR) -DCMAKE_BUILD_TYPE=Release
	cmake --build $(CPP_BUILD_DIR) -- -j$$(nproc)

cpp-run: cpp-build
	@echo "=== C++: 运行 ==="
	$(CPP_BIN) $(ARGS)

cpp-test: cpp-build
	@echo "=== C++: 测试 ==="
	cd $(CPP_BUILD_DIR) && ctest --output-on-failure

cpp-clean:
	rm -rf $(CPP_BUILD_DIR)

# ─── Rust ─────────────────────────────────────────────────────────
RUST_DIR    = rust
RUST_BIN    = $(RUST_DIR)/target/release/todo

.PHONY: rust-build rust-run rust-test rust-clean

rust-build:
	@echo "=== Rust: 构建 ==="
	cargo build --release --manifest-path $(RUST_DIR)/Cargo.toml

rust-run: rust-build
	@echo "=== Rust: 运行 ==="
	$(RUST_BIN) $(ARGS)

rust-test:
	@echo "=== Rust: 测试 ==="
	cargo test --manifest-path $(RUST_DIR)/Cargo.toml

rust-clean:
	cargo clean --manifest-path $(RUST_DIR)/Cargo.toml

# ─── Go ───────────────────────────────────────────────────────────
GO_DIR    = go
GO_BIN    = $(GO_DIR)/todo

.PHONY: go-build go-run go-test go-clean

go-build:
	@echo "=== Go: 构建 ==="
	cd $(GO_DIR) && go build -o todo .

go-run: go-build
	@echo "=== Go: 运行 ==="
	$(GO_BIN) $(ARGS)

go-test:
	@echo "=== Go: 测试 ==="
	cd $(GO_DIR) && go test ./...

go-clean:
	rm -f $(GO_BIN)

# ─── TypeScript ───────────────────────────────────────────────────
TS_DIR = ts

.PHONY: ts-build ts-run ts-test ts-clean

ts-build:
	@echo "=== TypeScript: 构建 ==="
	cd $(TS_DIR) && npm install && npx tsc

ts-run: ts-build
	@echo "=== TypeScript: 运行 ==="
	node $(TS_DIR)/dist/index.js $(ARGS)

ts-test:
	@echo "=== TypeScript: 测试 ==="
	cd $(TS_DIR) && npm test

ts-clean:
	rm -rf $(TS_DIR)/dist $(TS_DIR)/node_modules

# ─── Python ───────────────────────────────────────────────────────
PYTHON_DIR = python

.PHONY: py-build py-run py-test py-clean

py-build:
	@echo "=== Python: setup ==="

py-run:
	@echo "=== Python: 运行 ==="
	python -m todo_cli $(ARGS)

py-test:
	@echo "=== Python: 测试 ==="
	cd $(PYTHON_DIR) && python -m pytest

py-clean:
	@echo "=== Python: clean (no artifacts) ==="
	find $(PYTHON_DIR) -type d -name __pycache__ -exec rm -rf {} + 2>/dev/null; true
	find $(PYTHON_DIR) -type f -name '*.pyc' -delete

# ─── Shell ────────────────────────────────────────────────────────
SHELL_DIR = shell

.PHONY: sh-build sh-run sh-test sh-clean

sh-build:
	@echo "=== Shell: no build needed ==="

sh-run:
	@echo "=== Shell: 运行 ==="
	bash $(SHELL_DIR)/todo.sh $(ARGS)

sh-test:
	@echo "=== Shell: 测试 ==="
	bash $(SHELL_DIR)/test_todo.sh

sh-clean:
	@echo "=== Shell: clean (no artifacts) ==="

# ─── 聚合目标 ──────────────────────────────────────────────────────

LANGS = cpp rust go ts py sh

define LANG_TARGET
build-$(1):
	$(MAKE) $(1)-build

run-$(1):
	$(MAKE) $(1)-run ARGS="$(ARGS)"

test-$(1):
	$(MAKE) $(1)-test

clean-$(1):
	$(MAKE) $(1)-clean
endef

$(foreach lang,$(LANGS),$(eval $(call LANG_TARGET,$(lang))))

# 如果指定了 LANG，只构建该语言；否则构建所有
ifeq ($(LANG),all)
build: $(addsuffix -build,$(LANGS))
run: $(addsuffix -run,$(LANGS))
test: $(addsuffix -test,$(LANGS))
clean: $(addsuffix -clean,$(LANGS))
else
build: $(LANG)-build
run: $(LANG)-run
test: $(LANG)-test
clean: $(LANG)-clean
endif

.PHONY: build run test clean $(foreach lang,$(LANGS),$(lang)-build $(lang)-run $(lang)-test $(lang)-clean)

# VoiletMark

一个轻量的跨平台 Markdown 编辑器，基于 **Qt 5 / C++17** 构建。

当前版本 **0.12.6**。

## 界面

窗口从上到下依次是菜单栏、侧栏 + 编辑区、状态栏。菜单栏依次是
**文件 / 编辑 / 视图 / 格式 / 最近文件 / 最近目录 / 帮助**。

| 浅色 · 编辑模式 | 深色 · 编辑模式 |
| --- | --- |
| ![浅色主题](docs/screenshot-light.png) | ![深色主题](docs/screenshot-dark.png) |

源代码模式：等宽字体、行号、每行一条细线。

![源代码模式](docs/screenshot-source.png)

## 功能

### 编辑

- Markdown 语法高亮：标题、粗体 / 斜体 / 删除线、行内代码、围栏代码块、
  链接与图片、引用、有序 / 无序列表、任务列表、分隔线
- `Ctrl+1` ~ `Ctrl+6` 把选中行设成 1–6 级标题，再按一次变回正文
- `Ctrl+/` 把选中内容包成围栏代码块，再按一次拆掉
- 默认「编辑模式」隐藏 Markdown 标记、标题按级别放大，光标所在行露出原始标记
- 行号栏、当前行高亮
- 围栏代码块显示成带圆角边框的面板，右上角有「复制」按钮
  （源代码模式下不显示这些装饰）
- 代码块按语言做语法高亮：C / C++ / Python / JavaScript / TypeScript /
  Java / Rust / Go / JSON / Bash / SQL / YAML，关键字、字符串、数字、
  注释、预处理指令分色；不认识的标记语言就按等宽纯文本显示
- 代码框**右下角**是语言下拉菜单，选一下就会改写围栏上的语言标记，
  高亮立刻跟着变（选「纯文本」则去掉语言）；只有一行代码时它挪到复制按钮左边
- 表格渲染成带网格的表格控件：双击单元格直接编辑，右键可**插入 / 删除行、插入 / 删除列**，
  改完立刻回写成 Markdown 表格，对齐方式（`:---:`）也会保留
- 源代码模式下表格退回原始 Markdown 文本
- `Tab` / `Shift+Tab` 整行缩进，选中多行时批量缩进
- 自动换行（`Alt+Z`）、`Ctrl+滚轮` 或 `Ctrl++` / `Ctrl+-` 缩放字号
- 撤销 / 重做、剪切 / 复制 / 粘贴，菜单项会跟随可用状态自动灰显
- 把 Markdown 文件直接拖进窗口即可打开

### 文件夹侧栏

- **文件 → 打开文件夹…**（`Ctrl+Shift+O`）选一个目录，递归扫描
  `*.md` / `*.markdown` / `*.mdown` / `*.mkd`
- 侧栏顶部两个页签：**文件**（默认）/ **目录**
- **目录**页把当前文档的 1–6 级标题列成大纲（代码块里的 `#` 不算），
  按层级缩进，点一下跳到对应行；编辑时自动刷新
- 状态栏左下角的「侧栏」按钮或 `Ctrl+B` 显示 / 隐藏
- 点文件名即打开，当前文件在列表里高亮
- 每 5 秒自动扫描一次：目录里新增 / 删除的 Markdown 会自动出现在列表里
- 上次打开的文件夹会被记住，下次启动自动恢复

### Git

打开的文件夹如果是个 git 仓库，侧栏底部会出现一行 Git 工具条（显示当前分支）：

| 按钮 | 实际执行 |
| --- | --- |
| 获取 | `git fetch --all --prune` |
| 拉取 | `git pull --ff-only` |
| 添加 | `git add -A` |
| 提交 | 填说明后 `git commit -m` |
| 推送 | `git push` |

排列是两行：第一行 获取 / 拉取 / 添加，第二行 提交 和 推送。

右键侧栏里的文件有三项：

- **添加** → `git add -- <file>`，只暂存这一个文件
- **忽略** → 把路径写进 `.gitignore`（不改已跟踪的文件，也不碰远端）
- **删除远端** → `git rm --cached` + 提交 + 推送。**远端会真的删掉这个文件**，
  本地文件保留；执行前弹确认框。

所有命令都是异步跑的，不会卡界面；失败时弹窗显示 git 的原始输出。

### 编辑模式 / 源代码模式

默认是**编辑模式**，按“写字”的排版来：

- 正文用比例字体，整列限制在 760px 内**居中**，像一页文档
- 不显示行号栏
- 标题按级别放大加粗、粗体真加粗、行内代码带底色、链接只留文字
- Markdown 标记默认不显示；光标移到哪一行，那一行才露出 `#`、`**` 这些标记
- 段落统一行高 150%、段间留白

点左下角的 **`<src>`**（或 `Ctrl+Shift+M`）切到**源代码模式**：

- 等宽字体、行号栏、**每行一条细线**，通栏铺满
- 所有 Markdown 标记原样显示，只做着色，不放大标题
- 适合核对格式、对齐、复制纯文本

按钮在源码模式下会变成 `</src>`。

### 导出 PDF

**文件 → 导出 PDF…**（`Ctrl+E`）直接弹保存对话框，选个位置就导出，没有中间预览。
A4 纸、16mm 页边距。

Linux 上文件对话框强制用 Qt 自带的实现（`DontUseNativeDialog`）：
GTK 原生选择器在 dconf / gvfs 不可用时会导致「点导出没反应」。
Windows / macOS 保留系统原生对话框。

导出用的是 Qt 自带的 `QPdfWriter` + `QTextDocument`，不需要额外的 PDF 库，
也不经过任何平台的打印子系统（Windows 打印后台、CUPS），所以三端行为一致。
支持标题、列表（含任务列表）、引用、代码块、行内代码、粗体 / 斜体 / 删除线、
链接、表格；图片只输出 alt 文字。

- 代码块是**一整块带边框的面板**（不是一行一条底色），右上角标注语言名，
  块内按语言做语法高亮
- 表格渲染成真正的表格：表头灰底加粗、单元格边框、按 `:---:` 保留对齐

### 语言

界面支持中文和英文，在 **视图 → 语言** 里切换，立即生效、不需要重启。
选择会被记住。

- 界面文案的中文就是源码本身；英文由 `translations/voiletmark_en.ts` 提供
- Qt 自带的对话框（文件选择、消息框按钮）也会跟着切换，
  中文用的是 Qt 官方的翻译文件

### 文件

- 新建、打开、保存、另存为
- **最近文件** / **最近目录**：菜单栏上各有一个下拉菜单，各记 8 条，两个列表可以分别清空
- **启动时自动打开上次编辑的文件**；文件已经被删掉就退回欢迎文档
- 保留原文件的 **BOM** 与 **CRLF** 换行风格，保存时原子写入（`QSaveFile`）
- 关闭或切换文件时提示未保存的修改
- 记忆窗口位置、字号、换行开关、侧栏状态等设置

### 配置文件

所有设置都写进同一个 INI 文件，换机器时把它拷过去就行：

| 平台 | 路径 |
| --- | --- |
| Windows | `%LOCALAPPDATA%\VoiletMark\VoiletMark\config.ini` |
| Linux | `~/.config/VoiletMark/VoiletMark/config.ini` |
| macOS | `~/Library/Preferences/VoiletMark/VoiletMark/config.ini` |

确切路径也显示在 **帮助 → 关于 VoiletMark** 里。文件里记着主题、语言、字号、
自动换行、侧栏状态、最近文件 / 最近目录、上次打开的文件和上次的文件夹。

### 主题

四种模式：

| 模式 | 说明 |
| --- | --- |
| 跟随系统（默认） | 读取桌面环境的浅色 / 深色设置 |
| 浅色 | 固定浅色 |
| 深色 | 固定深色 |
| 按时段自动 | 6:00–18:00 浅色，其余时间深色 |

跟随系统时每 2 秒检测一次，改动后整套界面立即重绘：

| 平台 | 检测方式 |
| --- | --- |
| Windows | 注册表 `AppsUseLightTheme` |
| macOS | `defaults read -g AppleInterfaceStyle` |
| Linux | gsettings `color-scheme` / GTK 主题名 → KDE `kdeglobals` → XFCE `xsettings.xml` → 调色板兜底 |

模式可在 **视图 → 主题** 中切换，选择会被记住。

## 构建

依赖：Qt 5.12+（含 Widgets 模块）、支持 C++17 的编译器。

### Qt Creator（推荐）

1. 用 Qt Creator 打开根目录的 `VoiletMark.pro`
2. 在 **项目 → 构建设置** 中选好本机的 Qt 5.15.2 套件（Kit）
3. `Ctrl+R` 直接运行

Qt Creator 默认走影子构建（shadow build），不会污染源码目录。

### 命令行

```bash
mkdir -p build && cd build
$HOME/Qt5/5.15.2/gcc_64/bin/qmake ../VoiletMark.pro
make -j"$(nproc)"
./VoiletMark
```

或者直接：

```bash
./scripts/build.sh
```

脚本会依次查找 `command -v qmake`、`$QT_PREFIX/bin/qmake`、
`$HOME/Qt5/5.15.2/gcc_64/bin/qmake`；也可以用 `QMAKE=/path/to/qmake` 直接指定。

## 安装（Linux）

```bash
make -C build install                       # 默认装到 /usr/local
make -C build install INSTALL_ROOT=/tmp/pkg # 打包用，装到 /tmp/pkg/usr/local
```

会一起装上可执行文件、`voiletmark.desktop` 和 256×256 图标，
装完就能在应用菜单里搜到 VoiletMark。

## 打包（安装包）

`.github/workflows/packages.yml` 是给 GitHub 镜像用的：推代码、打 tag、PR 或手动运行时编译，
并打出下面这些安装包。

**Gitea 也会读这个目录**：Gitea Actions 兼容 GitHub，默认同时扫描 `.gitea/workflows` 和
`.github/workflows`。工作流里每个 job 都加了 `github.server_url == 'https://github.com'` 的守卫，
在 Gitea 上只会显示 skipped、不占 runner；要连 Actions 列表里的 run 都不出现，任选一个：

- 仓库 **设置 → Actions → General**，取消勾选 **Enable Actions** 后保存（整个仓库的 Actions 全停）
- 或 **Actions → 安装包** 页面点 **Disable Workflow**（只停这一个工作流）
- 或服务器 `app.ini` 里设 `[actions] WORKFLOW_DIRS = .gitea/workflows`，Gitea 就只读
  `.gitea/workflows`，`.github/workflows` 只对 GitHub 生效

Gitea 1.27 也可以直接调 API（`TOKEN` 换成你的访问令牌，`<id>` 用列表里返回的值）：

```bash
curl -H "Authorization: token $TOKEN" \
  https://git.linuxacme.com/api/v1/repos/liyepan/VoiletMark/actions/workflows
curl -X PUT -H "Authorization: token $TOKEN" \
  https://git.linuxacme.com/api/v1/repos/liyepan/VoiletMark/actions/workflows/<id>/disable
```

| 平台 | runner | 产物 |
| --- | --- | --- |
| Linux x86_64 | `ubuntu-22.04` | `voiletmark_<版本>_amd64.deb`、`voiletmark-<版本>-1.x86_64.rpm` |
| Linux aarch64 | `ubuntu-24.04-arm` | `..._arm64.deb`、`...aarch64.rpm` |
| Windows x86_64 | `windows-2022` | `VoiletMark-<版本>-win64-setup.exe`、`...-win64-portable.zip` |
| macOS arm64 | `macos-15` | `VoiletMark-<版本>-macos-arm64.dmg` |
| macOS x86_64 | `macos-15-intel` | `VoiletMark-<版本>-macos-x86_64.dmg` |

说明：

- Windows 安装包按当前用户装到 `%LOCALAPPDATA%\Programs\VoiletMark`，不需要管理员权限；
  会把 `.md` 关联到 VoiletMark，卸载时清理快捷方式、注册表和（仍指向本程序的）关联
- macOS 的 arm64 包是**原生**的，不依赖 Rosetta；Intel 包在 `macos-15-intel` 上用 Qt 5.15.2 官方二进制打
- 两个 dmg 都没有签名 / 公证：第一次打开要右键 →「打开」，或去「系统设置 → 隐私与安全性」点「仍要打开」
- Windows 安装包也没签名，SmartScreen 会拦一下（更多信息 → 仍要运行）
- 如果 .md 在资源管理器里还指向别的程序：右键 →「打开方式」→「选择其他应用」→ 勾选用 VoiletMark 打开（Win8 之后 UserChoice 不允许安装包直接改）
- Linux ARM 和 macOS Intel 的 runner 不是所有仓库都有；拿不到时那几个 job 会失败，不影响其它产物
- 某个平台失败时 Release 仍会带上其它平台成功的包

### 发 Release

- 推 main、打 tag、手动 Run workflow 都会**自动发 Release**（PR 不发）
- tag 用 `v<版本>`，版本号在 `VoiletMark.pro` 的 `VERSION` 里；同一版本多次推送会更新同一个 Release 的安装包
- 想发新版本就改 `VERSION`，下一次推送会自动建 `v<新版本>` 的 Release
- Release 是正式发布（不是草稿 / 预发布），并设为 Latest

### 本地打包（Linux）

版本号从 `VoiletMark.pro` 里取：

```bash
mkdir -p build && cd build
qmake ../VoiletMark.pro PREFIX=/usr && make -j"$(nproc)"
make install INSTALL_ROOT="$PWD/stage"

version=$(sed -n 's/^VERSION[[:space:]]*=[[:space:]]*//p' ../VoiletMark.pro | head -n1)
../packaging/build-deb.sh stage "$version" amd64 ../dist
../packaging/build-rpm.sh stage "$version" x86_64 ../dist
```

装包：

```bash
sudo apt install ./voiletmark_*_amd64.deb
sudo dnf install ./voiletmark-*.x86_64.rpm
```

打 rpm 需要 `rpmbuild`（Ubuntu 上 `apt install rpm`）。rpm 的 `License` 字段现在写的是
`Proprietary`（仓库里还没放 LICENSE 文件），要换许可证改 `packaging/build-rpm.sh` 里那一行即可。
Windows / macOS 的安装包建议直接用 CI 打（本地需要 NSIS、Xcode / Homebrew Qt），这里不再展开。

## 图标

`logo.png` 是源文件，`packaging/generate_icons.py` 从它生成各平台图标。
源图是**黑色字标 + 透明背景**，直接做成图标放进深色任务栏会“消失”，
所以脚本会给它垫一块白色圆角底板，并输出多个尺寸（16–256）：

| 文件 | 平台 |
| --- | --- |
| `voiletmark.ico` | Windows 可执行文件图标（`RC_ICONS`） |
| `voiletmark.icns` | macOS 应用包图标（`ICON`） |
| `voiletmark.png` | Linux 桌面图标（hicolor 256×256） |

运行时的窗口图标直接嵌在资源里（`resources.qrc`），所以不安装也有图标。
改了 `logo.png` 之后跑一次 `python3 packaging/generate_icons.py` 重新生成即可。

## 命令行

```bash
VoiletMark                     # 打开编辑器（显示欢迎文档）
VoiletMark notes.md            # 直接打开指定文件
VoiletMark --theme dark        # 本次运行使用深色主题（system / light / dark / schedule）
VoiletMark --screenshot a.png  # 渲染窗口截图后退出（调试 / 视觉回归）
```

## 快捷键

| 快捷键 | 功能 |
| --- | --- |
| `Ctrl+N` / `Ctrl+O` / `Ctrl+S` | 新建 / 打开 / 保存 |
| `Ctrl+Shift+S` | 另存为 |
| `Ctrl+Shift+O` | 打开文件夹 |
| `Ctrl+E` | 导出 PDF |
| `Ctrl+B` | 显示 / 隐藏侧栏 |
| `Ctrl+Shift+M` | 源代码模式 |
| `Ctrl+1` ~ `Ctrl+6` | 设为 1–6 级标题（再按取消） |
| `Ctrl+/` | 代码块（再按拆掉） |
| `Ctrl+Z` / `Ctrl+Shift+Z` | 撤销 / 重做 |
| `Ctrl+滚轮`、`Ctrl++`、`Ctrl+-`、`Ctrl+0` | 缩放 / 恢复字号 |
| `Alt+Z` | 切换自动换行 |
| `Tab` / `Shift+Tab` | 缩进 / 反缩进 |
| `Ctrl+Q` | 退出 |

## 代码结构

```
src/
├── main.cpp               入口：命令行解析、Fusion 样式、主题 / 语言初始化
├── ThemeManager.{h,cpp}   四种主题模式 + 系统检测 + 调色板 / QSS 生成
├── LanguageManager.{h,cpp} 中英文切换
├── AppConfig.{h,cpp}      所有设置共用的 config.ini
├── MainWindow.{h,cpp}     菜单、文件读写、侧栏、Git 工具栏、状态栏、设置
├── MarkdownEditor.{h,cpp} 编辑器控件：行号、缩进、代码框装饰、源码模式
├── MarkdownHighlighter.{h,cpp}  Markdown 着色与就地渲染
├── CodeHighlight.{h,cpp}  多语言语法高亮 tokenizer
├── MarkdownRenderer.{h,cpp} 导出用的 Markdown → 富文本文档转换
└── GitRunner.{h,cpp}      异步执行 git 命令

logo.png              图标源文件
resources.qrc         窗口图标与翻译资源
translations/         中英文翻译（.ts / .qm）
packaging/            图标、.desktop、deb/rpm、NSIS、macOS dmg 打包脚本
.github/workflows/    GitHub Actions：编译 + 打三平台安装包
```

## 下一版计划

- 查找 / 替换
- 列表按回车自动续行
- 表格与数学公式支持
- 导出 HTML
- HTML / CSS 的语法高亮

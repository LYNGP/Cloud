#!/bin/bash

set -e  # 出错就退出

# 颜色定义
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# 获取当前分支
current_branch=$(git rev-parse --abbrev-ref HEAD)

# 检查是否在 git 仓库中
if ! git rev-parse --is-inside-work-tree > /dev/null 2>&1; then
    echo -e "${RED}[ERROR] 当前目录不是 Git 仓库${NC}"
    exit 1
fi

# 检查是否有未跟踪或被忽略的敏感文件
echo -e "${YELLOW}[INFO] 检查工作区状态...${NC}"

# 检查是否有 .gitignore
if [ ! -f .gitignore ]; then
    echo -e "${YELLOW}[WARN] 未找到 .gitignore 文件，建议创建${NC}"
    read -p "是否继续？(y/n): " continue_without_gitignore
    if [ "$continue_without_gitignore" != "y" ]; then
        exit 0
    fi
fi

# 显示将要添加的文件
echo -e "${YELLOW}[INFO] 以下文件将被添加：${NC}"
git status --short

# 检查是否有未暂存的更改
if [ -z "$(git status --porcelain)" ]; then
    echo -e "${YELLOW}[WARN] 没有更改需要提交${NC}"
    exit 0
fi

# 确认是否继续
read -p "确认添加以上所有文件？(y/n): " confirm_add
if [ "$confirm_add" != "y" ]; then
    echo -e "${YELLOW}[INFO] 操作已取消${NC}"
    exit 0
fi

# 获取提交信息
msg="update $(date +'%Y%m%d %H:%M:%S')"
if [ -n "$1" ]; then
    msg="$1"
else
    echo -e "${YELLOW}[INFO] 未提供提交信息，使用默认信息：$msg${NC}"
    read -p "是否自定义提交信息？(y/n): " custom_msg
    if [ "$custom_msg" = "y" ]; then
        read -p "请输入提交信息: " msg
        if [ -z "$msg" ]; then
            echo -e "${RED}[ERROR] 提交信息不能为空${NC}"
            exit 1
        fi
    fi
fi

# 添加所有更改（不使用 git reset，保留已有的暂存状态）
echo -e "${GREEN}[INFO] 添加所有更改...${NC}"
git add .

# 再次显示将要提交的内容
echo -e "${YELLOW}[INFO] 将要提交的更改：${NC}"
git status --short

# 提交
echo -e "${GREEN}[INFO] 提交: $msg${NC}"
if ! git commit -m "$msg"; then
    echo -e "${RED}[ERROR] 提交失败${NC}"
    exit 1
fi

# 检查远程分支是否存在
if ! git ls-remote --exit-code --heads origin "$current_branch" > /dev/null 2>&1; then
    echo -e "${YELLOW}[WARN] 远程分支 '$current_branch' 不存在${NC}"
    read -p "是否创建并推送到新分支？(y/n): " create_branch
    if [ "$create_branch" != "y" ]; then
        echo -e "${YELLOW}[INFO] 操作已取消，本地提交已保存${NC}"
        exit 0
    fi
    echo -e "${GREEN}[INFO] 推送到新分支 $current_branch...${NC}"
    git push -u origin "$current_branch"
    echo -e "${GREEN}[INFO] ✅ 推送完成${NC}"
    exit 0
fi

# 同步远程最新代码
echo -e "${GREEN}[INFO] 同步远程最新代码 (分支: $current_branch)...${NC}"
if ! git pull --rebase origin "$current_branch"; then
    echo -e "${RED}[ERROR] 拉取远程代码时发生冲突${NC}"
    echo -e "${YELLOW}[INFO] 请手动解决冲突后执行：${NC}"
    echo -e "${YELLOW}  git rebase --continue${NC}"
    echo -e "${YELLOW}  git push origin $current_branch${NC}"
    exit 1
fi

# 推送到远端
echo -e "${GREEN}[INFO] 推送到远端 $current_branch 分支...${NC}"
if ! git push origin "$current_branch"; then
    echo -e "${RED}[ERROR] 推送失败${NC}"
    exit 1
fi

echo -e "${GREEN}[INFO] ✅ 所有操作完成！${NC}"
echo -e "${GREEN}[INFO] 分支: $current_branch${NC}"
echo -e "${GREEN}[INFO] 提交信息: $msg${NC}"
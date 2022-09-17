# Git版本控制

*❓由一个很神奇的问题展开，git是什么，它仅仅是一个工具嘛？*

> Git 是一个版本控制系统。
>
> Git 可帮助您跟踪代码更改。
>
> Git 用于在代码上进行协作。
>
> Git 思想可以带入你的工程(docker | hub | ipfs | ……所有设计版本机制)

**在这里面，可以掌握git的基本操作，基本可以解决百分之九十的开发问题，而剩下的百分之十，对应的最新高级篇可以帮你。**

**又或许，我们希望掌握它的原理，或许我们可以从头构建一个git，不是嘛🎉**

## 建立你自己的`Git`

- [**Haskell**：在 Haskell 中自下而上重新实现 “git clone”](http://stefan.saasen.me/articles/git-clone-in-haskell-from-the-bottom-up/)
- [**JavaScript** : 小程序](http://gitlet.maryrosecook.com/docs/gitlet.html)
- [**JavaScript** : 制造 GIT - 学习 GIT](https://kushagra.dev/blog/build-git-learn-git/)
- [**Python** : 一个 Git 客户端就其创建、提交并转发到 GitHub 上](https://benhoyt.com/writings/pygit/)
- [**Python**：给自己写一个 Git！](https://wyag.thb.lt/)
- [**Ruby** : 在 Ruby 中重建 Git](https://robots.thoughtbot.com/rebuilding-git-in-ruby)



## 实现git

> 为了实施`git clone`，将涵盖以下领域：
>
> - **git 协议**用于使git 客户端能够使用各种传输机制（本机 git 协议、ssh、http）从远程 git 服务器检索所需的更改集，
> - **包文件**格式，用于将最少量的数据从服务器传输到客户端（更普遍地用于有效地将打包对象存储在存储库中），
> - git 用于以 blob 形式存储提交、树、标签和实际文件内容的底层**对象存储**
> - 以及用于跟踪工作目录中文件更改的**索引格式。**

+ [ ] 如何实现`clone`





## 最新 – 进阶

> git基础篇会解决我们日常的百分之九十的问题，但是总是有那么一些问题需要我们注意的。比如说某一个，我开发的项目的某一个模块感觉这个模块没有之前的好了，那么我想切换回以前的模块。这个时候我们可以用`git log 指定项目目录` 得到以前的提交记录，然后`git switch 想要的版本hash \指定的目录` 最后就是`git commit -m "checkout ...."` 。

> 2022年9月10日 17:47:28
>
> **进阶篇存在的目的或许不仅仅是git语法层面的，就比如说我们需要删除一个分支，可能很简单的两个命令就可以解决**
>
> ```bash
> # 用两行命令删除分支
> ## 删除本地分支
> git branch -d localBranchName
> 
> ## 删除远程分支
> git push origin --delete remoteBranchName
> ```
>
> **但是我们很疑惑它们的用法**🗒️❌
>
> **1. 什么时候需要删除分支**
>
> 一个 Git 仓库常常有不同的分支，开发者可以在各个分支处理不同的特性，或者在不影响主代码库的情况下修复 bug。
>
> 仓库常常有一个 `master` 分支，表示主代码库。开发人员创建其他分支，处理不同的特性。
>
> 开发人员完成处理一个特性之后，常常会删除相应的分支。
>
> **2. 本地删除分支**
>
> 如果你还在一个分支上，那么 Git 是不允许你删除这个分支的。所以，请记得退出分支：`git checkout master`。
>
> 通过 `git branch -d <branch>`删除一个分支，比如：`git branch -d fix/authentication`。
>
> 当一个分支被推送并合并到远程分支后，`-d` 才会本地删除该分支。如果一个分支还没有被推送或者合并，那么可以使用`-D`强制删除它。
>
> 这就是本地删除分支的方法。
>
> **3. 删除远程分支**
>
> 使用这个命令可以远程删除分支：`git push <remote> --delete <branch>`。
>
> 比如： `git push origin --delete fix/authentication`，这个分支就被远程删除了。
>
> 你也可以使用这行简短的命令来远程删除分支：`git push <remote> :<branch>`，比如：`git push origin :fix/authentication`。
>
> 如果你得到以下错误消息，可能是因为其他人已经删除了这个分支。
>
> ```bash
> error: unable to push to unqualified destination: remoteBranchName The destination refspec neither matches an existing ref on the remote nor begins with refs/, and we are unable to guess a prefix based on the source ref. error: failed to push some refs to 'git@repository_name'
> ```
>
> 使用以下命令同步分支列表：
>
> ```
> git fetch -p
> ```
>
> `-p` 的意思是“精简”。这样，你的分支列表里就不会显示已远程被删除的分支了。

+ [x] [😎🎉Git高级部分](./markdown/super.md)
+ [x] [😎🎉移动提交记录 — 自由修改提交树](./markdown/move.md)
+ [x] [😎🎉Git提交技巧](./markdown/commit.md)
+ [x] [😎🎉Git — tags](./markdown/tags.md)
+ [x] [😎🎉Git — 多次rebase](./markdown/rebase.md)

+ [x] [😎🎉Git — module](./markdown/module.md)

+ [x] [😎🎉Git — Script自动推送脚本](./markdown/git-script.md)

> `git - module`不仅仅很有用，而且里面的实现也值得学习
>
> `git - Script`脚本能让我们更快的、更方便推动项目



## 基础部分

> 有时候我们需要撤销上一次的`add`或者`commit`怎么办?
>
> + 我们使用`git reset HEAD` 这样只是撤销了上一次`add`
> + 我们使用：`git reset --soft HEAD^` 这样就成功撤销了`commit`。
> + 使用`git reset --hard HEAD^` 这样连`add`也撤销了。

+ [x] 🗝️ [1. github基础命令大全](./markdown/github基础知识.md)

+ [x] 🗝️ [2. git和svn对比](./markdown/git和svn对比.md)

+ [x] 🗝️ [3. git工作原理](./markdown/git工作原理.md)

+ [x] 🗝️ [4. git分支](./markdown/git分支.md)

+ [x] 🗝️ [5. git上传](./markdown/git上传.md)

+ [x] 🗝️ [6. git（菜鸟）](./markdown/菜鸟git.md)

+ [x] 🗝️ [7. git分支与合并](./markdown/git分支与合并.md)

+ [x] 🗝️ [8. git版本回退](./markdown/git版本回退.md)

+ [x] 🗝️  [9. git比较两个分支差异](./markdown/git比较两个分支差异.md)

+ [x] 🗝️ [10. Git忽略和 .gitignore](./markdown/Git忽略和gitignore.md)

+ [x] 🗝️ [11. 提交到多个远程仓库](./markdown/git-adds.md)

+ [x] 🗝️ [12. 贡献代码](./markdown/git-contributor.md)


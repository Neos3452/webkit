
class TestGroupRevisionTable extends ComponentBase {
    constructor()
    {
        super('test-group-revision-table');
        this._testGroup = null;
        this._analysisResults = null;
        this._renderTableLazily = new LazilyEvaluatedFunction(this._renderTable.bind(this));
    }

    setTestGroup(testGroup)
    {
        this._testGroup = testGroup;
        this.enqueueToRender();
    }

    setAnalysisResults(analysisResults)
    {
        this._analysisResults = analysisResults;
        this.enqueueToRender();
    }

    render()
    {
        this._renderTableLazily.evaluate(this._testGroup, this._analysisResults);
    }

    _renderTable(testGroup, analysisResults)
    {
        if (!testGroup)
            return;

        const commitSets = testGroup.requestedCommitSets();

        const requestedRepositorySet = new Set;
        const additionalRepositorySet = new Set;
        for (const commitSet of commitSets) {
            for (const repository of commitSet.repositories())
                requestedRepositorySet.add(repository);
        }

        const rowEntries = [];
        let hasCustomRoots = false;
        let firstRequest = null;
        commitSets.forEach((commitSet, commitSetIndex) => {
            const setLabel = testGroup.labelForCommitSet(commitSet);
            const buildRequests = testGroup.requestsForCommitSet(commitSet);

            if (commitSet.customRoots().length)
                hasCustomRoots = true;
            if (!firstRequest)
                firstRequest = buildRequests[0];

            buildRequests.forEach((request, i) => {
                const resultCommitSet = analysisResults ? analysisResults.commitSetForRequest(request) : null;
                if (resultCommitSet) {
                    for (const repository of resultCommitSet.repositories()) {
                        if (!requestedRepositorySet.has(repository))
                            additionalRepositorySet.add(repository);
                    }
                }
                let label = (1 + request.order()).toString();
                if (request.order() < 0)
                    label = `Build ${request.order() - firstRequest.order() + 1}`;
                else if (firstRequest.isBuild())
                    label = `Test ${label}`;
                rowEntries.push({
                    groupHeader: !i ? setLabel : null,
                    groupRowCount: buildRequests.length,
                    label,
                    commitSet: resultCommitSet || commitSet,
                    requestedCommitSet: commitSet,
                    customRoots: commitSet.customRoots(), // FIXME: resultCommitSet should also report roots that got installed.
                    rowCountByRepository: new Map,
                    repositoriesToSkip: new Set,
                    customRootsRowCount: 1,
                    request,
                });
            });
        });

        this._mergeCellsWithSameCommitsAcrossRows(rowEntries);

        const requestedRepositoryList = Repository.sortByNamePreferringOnesWithURL([...requestedRepositorySet]);
        const additionalRepositoryList = Repository.sortByNamePreferringOnesWithURL([...additionalRepositorySet]);

        const element = ComponentBase.createElement;
        const link = ComponentBase.createLink;
        this.renderReplace(this.content('revision-table'), [
            element('thead', [
                element('th', 'Configuration'),
                requestedRepositoryList.map((repository) => element('th', repository.name())),
                hasCustomRoots ? element('th', 'Roots') : [],
                element('th', 'Order'),
                element('th', 'Status'),
                additionalRepositoryList.map((repository) => element('th', repository.name())),
            ]),
            element('tbody', rowEntries.map((entry) => {
                const request = entry.request;
                return element('tr', [
                    entry.groupHeader ? element('td', {rowspan: entry.groupRowCount}, entry.groupHeader) : [],
                    requestedRepositoryList.map((repository) => this._buildCommitCell(entry, repository)),
                    hasCustomRoots ? this._buildCustomRootsCell(entry) : [],
                    element('td', entry.label),
                    element('td', request.statusUrl() ? link(request.statusLabel(), request.statusUrl()) : request.statusLabel()),
                    additionalRepositoryList.map((repository) => this._buildCommitCell(entry, repository)),
                ]);
            }))]);
    }

    _buildCommitCell(entry, repository)
    {
        const element = ComponentBase.createElement;
        const link = ComponentBase.createLink;

        if (entry.repositoriesToSkip.has(repository))
            return [];

        const commit = entry.commitSet.commitForRepository(repository);
        let content = '';
        if (commit) {
            content = commit.label();
            if (commit.url())
                content = link(content, commit.url());
        }

        const patch = entry.requestedCommitSet.patchForRepository(repository);
        if (patch)
            content = [content, ' with ', this._buildFileInfo(patch)];

        return element('td', {rowspan: entry.rowCountByRepository.get(repository)}, content);
    }

    _buildCustomRootsCell(entry)
    {
        const rowspan = entry.customRootsRowCount;
        if (!rowspan)
            return [];

        const element = ComponentBase.createElement;
        if (!entry.customRoots.length)
            return element('td', {class: 'roots', rowspan});

        return element('td', {class: 'roots', rowspan},
            element('ul', entry.customRoots.map((customRoot) => {
                return this._buildFileInfo(customRoot);
            }).map((content) => element('li', content))));
    }

    _buildFileInfo(file)
    {
        const element = ComponentBase.createElement;
        const link = ComponentBase.createLink;

        if (file.deletedAt())
            return [file.label(), ' ', element('span', {class: 'purged'}, '(Purged)')];
        return link(file.label(), file.url());
    }

    _mergeCellsWithSameCommitsAcrossRows(rowEntries)
    {
        for (let rowIndex = 0; rowIndex < rowEntries.length; rowIndex++) {
            const entry = rowEntries[rowIndex];
            for (const repository of entry.commitSet.repositories()) {
                if (entry.repositoriesToSkip.has(repository))
                    continue;
                const commit = entry.commitSet.commitForRepository(repository);
                const patch = entry.commitSet.patchForRepository(repository);
                let rowCount = 1;
                for (let otherRowIndex = rowIndex + 1; otherRowIndex < rowEntries.length; otherRowIndex++) {
                    const otherEntry = rowEntries[otherRowIndex];
                    const otherCommit = otherEntry.commitSet.commitForRepository(repository);
                    const otherPatch = otherEntry.commitSet.patchForRepository(repository);
                    if (commit != otherCommit || patch != otherPatch)
                        break;
                    otherEntry.repositoriesToSkip.add(repository);
                    rowCount++;
                }
                entry.rowCountByRepository.set(repository, rowCount);
            }
            if (entry.customRootsRowCount) {
                let rowCount = 1;
                for (let otherRowIndex = rowIndex + 1; otherRowIndex < rowEntries.length; otherRowIndex++) {
                    const otherEntry = rowEntries[otherRowIndex];
                    if (!CommitSet.areCustomRootsEqual(entry.customRoots, otherEntry.customRoots))
                        break;
                    otherEntry.customRootsRowCount = 0;
                    rowCount++;
                }
                entry.customRootsRowCount = rowCount;
            }
        }
    }

    static htmlTemplate()
    {
        return `<table id="revision-table"></table>`;
    }

    static cssTemplate()
    {
        return `
            table {
                border-collapse: collapse;
            }
            th, td {
                text-align: center;
                padding: 0.2rem 0.8rem;
            }
            tbody th,
            tbody td {
                border-top: solid 1px #eee;
                border-bottom: solid 1px #eee;
            }
            th {
                font-weight: inherit;
            }
            .roots {
                max-width: 20rem;
            }
            .purged {
                color: #999;
            }
            .roots ul,
            .roots li {
                list-style: none;
                margin: 0;
                padding: 0;
            }
            .roots li {
                margin-top: 0.4rem;
                margin-bottom: 0.4rem;
            }
        `;
    }
}

ComponentBase.defineElement('test-group-revision-table', TestGroupRevisionTable);
